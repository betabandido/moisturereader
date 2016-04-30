import Adafruit_BluefruitLE
from Adafruit_BluefruitLE.services import UART
import os
import Queue
import time
from threading import Thread

packet_queue = Queue.Queue()

class PacketProcessor(Thread):
  """Class for processing packets being sent from the sensor.

  The packet processor runs on its own threads and uses a queue to communicate
  with the thread responsible to receive packets over bluetooth.
  """

  def __init__(self, out_fname):
    """Initialization method.

    Args:
      out_fname: Output file name where to write the sensor samples.
    """
    Thread.__init__(self)
    self.out_fname = out_fname

  def run(self):
    """Thread's run() method."""
    try:
      with open(self.out_fname, 'w') as out_file:
        self.run_impl(out_file)
    except Exception, e:
      print 'Error: {}'.format(e)

  def run_impl(self, out_file):
    """Loop where packets are actually processed.

    Args:
      out_file: Output file object.
    """
    packet_buffer = []
    while True:
      global packet_queue
      char = packet_queue.get()
      packet_queue.task_done();
      if char == 'Q':
        break
      elif char == 'E':
        self.process_packet(packet_buffer, out_file)
        packet_buffer = []
      else:
        packet_buffer += [char]

  def process_packet(self, packet, out_file):
    """Processes a single packet.

    Args:
      packet: A character buffer containing the packet.
      out_file: Output file object.
    """
    if len(packet) == 0:
      raise Exception('Empty packet')
    if packet[0] != 'B':
      raise Exception('Malformed packet')

    packet = int(''.join(packet[1:]))

    print 'Sample: {}'.format(packet)
    print >>out_file, '{},{}'.format(time.time(), packet)
    out_file.flush()
    os.fsync(out_file.fileno())

class PacketReceiver:
  """This class is responsible for receiving packets from the sensor."""

  def __init__(self, ble):
    """Initialization method.

    Args:
      ble: The Bluetooth LE instance.
    """
    self.ble = ble

  def __call__(self):
    """Method call from the BLE event loop."""
    try:
      (device, uart) = self.connect_to_device()
    except:
      return

    try:
      while True:
        self.receive_data(uart);
    except Exception, e:
      print 'Error: {}'.format(e)
    finally:
      device.disconnect()

    global packet_queue
    packet_queue.join()

  def connect_to_device(self):
    """Connects to the device.

    Returns:
      A pair containing the device that we connected to and the UART instance.
    """
    self.ble.clear_cached_data()

    adapter = self.ble.get_default_adapter()
    adapter.power_on()
    print('Using adapter: {}'.format(adapter.name))

    print('Disconnecting any connected UART devices...')
    UART.disconnect_devices()

    print('Searching for UART device...')
    try:
      adapter.start_scan()
      device = UART.find_device()
      if device is None:
        raise RuntimeError('Failed to find UART device!')
    finally:
      adapter.stop_scan()

    print('Connecting to device...')
    device.connect()

    try:
      print('Discovering services...')
      UART.discover(device)
      uart = UART(device)
    except:
      device.disconnect()
      raise

    return (device, uart)

  def receive_data(self, uart):
    """Receives data from the Bluetooth LE device.

    Args:
      uart: The UART instance.
    """
    received = uart.read(timeout_sec=60)
    if received is None:
      packet_queue.put('Q')
      raise Exception('Received no data!')

    for c in received:
      packet_queue.put(c)

###

ble = Adafruit_BluefruitLE.get_provider()
ble.initialize()

packet_receiver = PacketReceiver(ble)

out_fname = os.path.join(os.path.expanduser('~'), 'tmp/moisture.csv')
packet_processor = PacketProcessor(out_fname)
packet_processor.start()

ble.run_mainloop_with(packet_receiver)

