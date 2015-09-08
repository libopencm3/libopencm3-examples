import array
import datetime
import usb.core
import usb.util as uu
import logging

import unittest

DUT_SERIAL = "stm32f4disco"
#DUT_SERIAL = "stm32f103-generic"
#DUT_SERIAL = "stm32l1-generic"
#DUT_SERIAL = "stm32f072disco"

class find_by_serial(object):
    def __init__(self, serial):
        self._serial = serial

    def __call__(self, device):
        return device.serial_number == self._serial


class TestGadget0(unittest.TestCase):
    # TODO - parameterize this with serial numbers so we can find
    # gadget 0 code for different devices.  (or use different PIDs?)
    def setUp(self):
        self.dev = usb.core.find(idVendor=0xcafe, idProduct=0xcafe, custom_match=find_by_serial(DUT_SERIAL))
        self.assertIsNotNone(self.dev, "Couldn't find locm3 gadget0 device")

    def tearDown(self):
        uu.dispose_resources(self.dev)

    def test_sanity(self):
        self.assertEqual(2, self.dev.bNumConfigurations, "Should have 2 configs")

    def test_config_switch_2(self):
        """
        Uses the API if you're interested in the cfg block
        """
        cfg = uu.find_descriptor(self.dev, bConfigurationValue=2)
        self.assertIsNotNone(cfg, "Config 2 should exist")
        self.dev.set_configuration(cfg)

    def test_config_switch_3(self):
        """
        Uses the simple API
        """
        self.dev.set_configuration(3)

    def test_fetch_config(self):
        self.dev.set_configuration(3)
        # FIXME - find a way to get the defines for these from pyusb
        x = self.dev.ctrl_transfer(0x80, 0x08, 0, 0, 1)
        self.assertEqual(3, x[0], "Should get the actual bConfigurationValue back")

    def test_invalid_config(self):
        """ Note, testing config(0) needs a valid config to test along side, see other tests"""
        try:
            # FIXME - find a way to get the defines for these from pyusb
            self.dev.ctrl_transfer(0x00, 0x09, 99)
            self.fail("Request of invalid cfg should have failed")
        except usb.core.USBError as e:
            # Note, this might not be as portable as we'd like.
            self.assertIn("Pipe", e.strerror)

class TestConfigSourceSink(unittest.TestCase):
    """
    We could inherit, but it doesn't save much, and this saves me from remembering how to call super.
    """

    def setUp(self):
        self.dev = usb.core.find(idVendor=0xcafe, idProduct=0xcafe, custom_match=find_by_serial(DUT_SERIAL))
        self.assertIsNotNone(self.dev, "Couldn't find locm3 gadget0 device")

        self.cfg = uu.find_descriptor(self.dev, bConfigurationValue=2)
        self.assertIsNotNone(self.cfg, "Config 2 should exist")
        self.dev.set_configuration(self.cfg);
        self.intf = self.cfg[(0, 0)]
        # heh, kinda gross...
        self.ep_out = [ep for ep in self.intf if uu.endpoint_direction(ep.bEndpointAddress) == uu.ENDPOINT_OUT][0]
        self.ep_in = [ep for ep in self.intf if uu.endpoint_direction(ep.bEndpointAddress) == uu.ENDPOINT_IN][0]

    def tearDown(self):
        uu.dispose_resources(self.dev)

    def test_write_simple(self):
        """
        here we go, start off with just a simple write of < bMaxPacketSize and just make sure it's accepted
        :return:
        """
        data = [x for x in range(int(self.ep_out.wMaxPacketSize / 2))]
        written = self.dev.write(self.ep_out, data)
        self.assertEqual(written, len(data), "Should have written all bytes plz")

    def test_write_zlp(self):
        written = self.ep_out.write([])
        self.assertEqual(0, written, "should have written zero for a zero length write y0")

    def test_write_batch(self):
        """
        Write 50 max sized packets.  Should not stall.  Will stall if firmware isn't consuming data properly
        :return:
        """
        for i in range(50):
            data = [x for x in range(int(self.ep_out.wMaxPacketSize))]
            written = self.dev.write(self.ep_out, data)
            self.assertEqual(written, len(data), "Should have written all bytes plz")

    def test_write_mixed(self):
        for i in range(int(self.ep_out.wMaxPacketSize / 4), self.ep_out.wMaxPacketSize * 10, 11):
            data = [x & 0xff for x in range(i)]
            written = self.ep_out.write(data)
            self.assertEqual(written, len(data), "should have written all bytes plz")

    def test_read_zeros(self):
        self.dev.ctrl_transfer(uu.CTRL_TYPE_VENDOR | uu.CTRL_RECIPIENT_INTERFACE, 0x1, 0)
        self.ep_in.read(self.ep_in.wMaxPacketSize)  # Clear out any prior pattern data
        # unless, you know _exactly_ how much will be written by the device, always read
        # an integer multiple of max packet size, to avoid overflows.
        # the returned data will have the actual length.
        # You can't just magically read out less than the device wrote.
        read_size = self.ep_in.wMaxPacketSize * 10
        data = self.dev.read(self.ep_in, read_size)
        self.assertEqual(len(data), read_size, "Should have read as much as we asked for")
        expected = array.array('B', [0 for x in range(read_size)])
        self.assertEqual(data, expected, "In pattern 0, all source data should be zeros: ")

    def test_read_sequence(self):
        # switching to the mod63 pattern requires resynching carefully to read out any zero frames already
        # queued, but still make sure we start the sequence at zero.
        self.dev.ctrl_transfer(uu.CTRL_TYPE_VENDOR | uu.CTRL_RECIPIENT_INTERFACE, 0x1, 1)
        self.ep_in.read(self.ep_in.wMaxPacketSize)  # Potentially queued zeros, or would have been safe.
        self.dev.ctrl_transfer(uu.CTRL_TYPE_VENDOR | uu.CTRL_RECIPIENT_INTERFACE, 0x1, 1)
        self.ep_in.read(self.ep_in.wMaxPacketSize)  # definitely right pattern now, but need to restart at zero.
        read_size = self.ep_in.wMaxPacketSize * 3
        data = self.dev.read(self.ep_in, read_size)
        self.assertEqual(len(data), read_size, "Should have read as much as we asked for")
        expected = array.array('B', [x % 63 for x in range(read_size)])
        self.assertEqual(data, expected, "In pattern 1, Should be % 63")

    def test_read_write_interleaved(self):
        for i in range(1, 20):
            ii = self.ep_in.read(self.ep_in.wMaxPacketSize * i)
            dd = [x & 0xff for x in range(i * 20 + 3)]
            oo = self.ep_out.write(dd)
            self.assertEqual(len(ii), self.ep_in.wMaxPacketSize * i, "should have read full packet")
            self.assertEqual(oo, len(dd), "should have written full packet")

    def test_control_known(self):
        self.dev.ctrl_transfer(uu.CTRL_TYPE_VENDOR | uu.CTRL_RECIPIENT_INTERFACE, 0x1, 0)
        self.dev.ctrl_transfer(uu.CTRL_TYPE_VENDOR | uu.CTRL_RECIPIENT_INTERFACE, 0x1, 1)
        self.dev.ctrl_transfer(uu.CTRL_TYPE_VENDOR | uu.CTRL_RECIPIENT_INTERFACE, 0x1, 99)
        self.dev.ctrl_transfer(uu.CTRL_TYPE_VENDOR | uu.CTRL_RECIPIENT_INTERFACE, 0x1, 0)

    def test_control_unknown(self):
        try:
            self.dev.ctrl_transfer(uu.CTRL_TYPE_VENDOR | uu.CTRL_RECIPIENT_INTERFACE, 42, 69)
            self.fail("Should have got a stall")
        except usb.core.USBError as e:
            # Note, this might not be as portable as we'd like.
            self.assertIn("Pipe", e.strerror)

    @unittest.skip("known failure in locm3 at present!")
    def test_unconfigure(self):
        """
        Actually testing set_config(0) but need to have a config to test it with as well
        """
        # we're already set in config valid.... write and check it worked.
        self.test_write_simple()
        # now unconfigure
        # FIXME - find a way to get the defines for these from pyusb
        self.dev.ctrl_transfer(0x00, 0x09, 0)
        # now try and write again...
        try:
            self.test_write_simple()
            self.fail("Should have failed to write again")
        except usb.core.USBError as e:
            # should be a timeout!
            print(e)
            self.assertIn("Timeout", e.strerror)


@unittest.skip("Perf tests only on demand (comment this line!)")
class TestConfigSourceSinkPerformance(unittest.TestCase):
    """
    Read/write throughput, roughly
    """

    def setUp(self):
        self.dev = usb.core.find(idVendor=0xcafe, idProduct=0xcafe, custom_match=find_by_serial(DUT_SERIAL))
        self.assertIsNotNone(self.dev, "Couldn't find locm3 gadget0 device")

        self.cfg = uu.find_descriptor(self.dev, bConfigurationValue=2)
        self.assertIsNotNone(self.cfg, "Config 2 should exist")
        self.dev.set_configuration(self.cfg);
        self.intf = self.cfg[(0, 0)]
        # heh, kinda gross...
        self.ep_out = [ep for ep in self.intf if uu.endpoint_direction(ep.bEndpointAddress) == uu.ENDPOINT_OUT][0]
        self.ep_in = [ep for ep in self.intf if uu.endpoint_direction(ep.bEndpointAddress) == uu.ENDPOINT_IN][0]

    def tearDown(self):
        uu.dispose_resources(self.dev)

    def tput(self, xc, te):
        return (xc / 1024 / max(1, te.seconds + te.microseconds /
                                1000000.0))

    def test_read_perf(self):
        # I get around 990kps here...
        ts = datetime.datetime.now()
        rxc = 0
        while rxc < 5 * 1024 * 1024:
            desired = 100 * 1024
            data = self.ep_in.read(desired, timeout=0)
            self.assertEqual(desired, len(data), "Should have read all bytes plz")
            rxc += len(data)
        te = datetime.datetime.now() - ts
        print("read %s bytes in %s for %s kps" % (rxc, te, self.tput(rxc, te)))

    def test_write_perf(self):
        # caps out around 420kps?
        ts = datetime.datetime.now()
        txc = 0
        data = [x & 0xff for x in range(100 * 1024)]
        while txc < 5 * 1024 * 1024:
            w = self.ep_out.write(data, timeout=0)
            self.assertEqual(w, len(data), "Should have written all bytes plz")
            txc += w
        te = datetime.datetime.now() - ts
        print("wrote %s bytes in %s for %s kps" % (txc, te, self.tput(txc, te)))
