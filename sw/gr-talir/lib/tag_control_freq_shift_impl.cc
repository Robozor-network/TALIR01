#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "tag_control_freq_shift_impl.h"

namespace gr {
  namespace talir {

    tag_control_freq_shift::sptr
    tag_control_freq_shift::make(float samp_rate)
    {
      return gnuradio::get_initial_sptr
        (new tag_control_freq_shift_impl(samp_rate));
    }

    /*
     * The private constructor
     */
    tag_control_freq_shift_impl::tag_control_freq_shift_impl(float samp_rate)
      : gr::sync_block("tag_control_freq_shift",
              gr::io_signature::make(1, 1, sizeof(gr_complex)),
              gr::io_signature::make(1, 1, sizeof(gr_complex))),
        samp_rate(samp_rate)
    {}

    /*
     * Our virtual destructor.
     */
    tag_control_freq_shift_impl::~tag_control_freq_shift_impl()
    {
    }

    int
    tag_control_freq_shift_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const gr_complex *in = (const gr_complex *) input_items[0];
      gr_complex *out = (gr_complex *) output_items[0];

      uint64_t off = nitems_read(0);

      std::vector<tag_t> tags;
      get_tags_in_window(tags, 0, 0, noutput_items, pmt::mp("freq_shift"));

      int pos = 0;
      int nremains = noutput_items;
      for (int i = 0; i <= tags.size(); i++) {
        int nitems = (i < tags.size()) ? (tags[i].offset - off - pos) : nremains;
        assert(nitems >= 0 && nitems <= nremains);
        d_r.rotateN(out, in, nitems);
        out += nitems; in += nitems; nremains -= nitems; pos += nitems;
        if (i < tags.size() && pmt::is_real(tags[i].value)) {
          d_r.set_phase_incr(exp(gr_complex(0, M_PI * 2 * pmt::to_float(tags[i].value) / samp_rate)));
        }
      }

      return noutput_items;
    }

  } /* namespace talir */
} /* namespace gr */
