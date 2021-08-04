#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: Rec 200Khz
# GNU Radio version: 3.7.14.0
##################################################

from gnuradio import blocks
from gnuradio import eng_notation
from gnuradio import filter
from gnuradio import gr
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from optparse import OptionParser
import iio


class rec_200khz(gr.top_block):

    def __init__(self, fn='', lofreq=4361e5, samprate=600e3, uri='ip:10.1.1.13'):
        gr.top_block.__init__(self, "Rec 200Khz")

        ##################################################
        # Parameters
        ##################################################
        self.fn = fn
        self.lofreq = lofreq
        self.samprate = samprate
        self.uri = uri

        ##################################################
        # Blocks
        ##################################################
        self.pluto_source_0 = iio.pluto_source(uri, int(lofreq), int(samprate), int(200000), 0x8000, True, True, True, "manual", 73, '', True)
        self.freq_xlating_fir_filter_xxx_0 = filter.freq_xlating_fir_filter_ccc(3, (firdes.low_pass(1.0, samprate, samprate/6, samprate/6/20)), -100e3, samprate)
        self.blocks_file_sink_1 = blocks.file_sink(gr.sizeof_gr_complex*1, fn, False)
        self.blocks_file_sink_1.set_unbuffered(False)



        ##################################################
        # Connections
        ##################################################
        self.connect((self.freq_xlating_fir_filter_xxx_0, 0), (self.blocks_file_sink_1, 0))
        self.connect((self.pluto_source_0, 0), (self.freq_xlating_fir_filter_xxx_0, 0))

    def get_fn(self):
        return self.fn

    def set_fn(self, fn):
        self.fn = fn
        self.blocks_file_sink_1.open(self.fn)

    def get_lofreq(self):
        return self.lofreq

    def set_lofreq(self, lofreq):
        self.lofreq = lofreq
        self.pluto_source_0.set_params(int(self.lofreq), int(self.samprate), int(200000), True, True, True, "manual", 73, '', True)

    def get_samprate(self):
        return self.samprate

    def set_samprate(self, samprate):
        self.samprate = samprate
        self.pluto_source_0.set_params(int(self.lofreq), int(self.samprate), int(200000), True, True, True, "manual", 73, '', True)
        self.freq_xlating_fir_filter_xxx_0.set_taps((firdes.low_pass(1.0, self.samprate, self.samprate/6, self.samprate/6/20)))

    def get_uri(self):
        return self.uri

    def set_uri(self, uri):
        self.uri = uri


def argument_parser():
    parser = OptionParser(usage="%prog: [options]", option_class=eng_option)
    parser.add_option(
        "", "--fn", dest="fn", type="string", default='',
        help="Set fn [default=%default]")
    parser.add_option(
        "", "--lofreq", dest="lofreq", type="eng_float", default=eng_notation.num_to_str(4361e5),
        help="Set lofreq [default=%default]")
    parser.add_option(
        "", "--samprate", dest="samprate", type="eng_float", default=eng_notation.num_to_str(600e3),
        help="Set samprate [default=%default]")
    parser.add_option(
        "", "--uri", dest="uri", type="string", default='ip:10.1.1.13',
        help="Set uri [default=%default]")
    return parser


def main(top_block_cls=rec_200khz, options=None):
    if options is None:
        options, _ = argument_parser().parse_args()

    tb = top_block_cls(fn=options.fn, lofreq=options.lofreq, samprate=options.samprate, uri=options.uri)
    tb.start()
    tb.wait()


if __name__ == '__main__':
    main()
