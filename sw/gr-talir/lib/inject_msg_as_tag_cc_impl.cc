/* -*- c++ -*- */
/* 
 * Copyright 2020 gr-talir author.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "inject_msg_as_tag_cc_impl.h"

namespace gr {
  namespace talir {

    inject_msg_as_tag_cc::sptr
    inject_msg_as_tag_cc::make()
    {
      return gnuradio::get_initial_sptr
        (new inject_msg_as_tag_cc_impl());
    }

    /*
     * The private constructor
     */
    inject_msg_as_tag_cc_impl::inject_msg_as_tag_cc_impl()
      : gr::sync_block("inject_msg_as_tag_cc",
              gr::io_signature::make(1, 1, sizeof(gr_complex)),
              gr::io_signature::make(1, 1, sizeof(gr_complex)))
    {
      message_port_register_in(pmt::mp("msg"));
      set_msg_handler(pmt::mp("msg"), boost::bind(&inject_msg_as_tag_cc_impl::handle_msg, this, _1));
    }

    void
    inject_msg_as_tag_cc_impl::handle_msg(pmt::pmt_t msg)
    {
      qu.push_back(msg);
    }

    /*
     * Our virtual destructor.
     */
    inject_msg_as_tag_cc_impl::~inject_msg_as_tag_cc_impl()
    {
    }

    int
    inject_msg_as_tag_cc_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const gr_complex *in = (const gr_complex *) input_items[0];
      gr_complex *out = (gr_complex *) output_items[0];

      while (!qu.empty()) {
        pmt::pmt_t &msg = qu[0];
        if (pmt::is_pair(msg))
          add_item_tag(0, nitems_read(0), pmt::car(msg), pmt::cdr(msg));
        qu.pop_front();
      }

      for (int i = 0; i < noutput_items; i++)
        out[i] = in[i];

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

  } /* namespace talir */
} /* namespace gr */

