#ifndef INCLUDED_TALIR_TAG_CONTROL_FREQ_SHIFT_IMPL_H
#define INCLUDED_TALIR_TAG_CONTROL_FREQ_SHIFT_IMPL_H

#include <gnuradio/blocks/rotator.h>
#include <talir/tag_control_freq_shift.h>

namespace gr {
  namespace talir {

    class tag_control_freq_shift_impl : public tag_control_freq_shift
    {
     private:
      gr::blocks::rotator d_r;
      float samp_rate;

     public:
      tag_control_freq_shift_impl(float samp_rate);
      ~tag_control_freq_shift_impl();

      // Where all the action really happens
      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items);
    };

  } // namespace talir
} // namespace gr

#endif /* INCLUDED_TALIR_TAG_CONTROL_FREQ_SHIFT_IMPL_H */

