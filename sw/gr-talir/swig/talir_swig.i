/* -*- c++ -*- */

#define TALIR_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "talir_swig_doc.i"

%{
#include "talir/tag_control_freq_shift.h"
#include "talir/clock_recovery_mm_ext_ff.h"
#include "talir/inject_msg_as_tag_cc.h"
%}


%include "talir/tag_control_freq_shift.h"
GR_SWIG_BLOCK_MAGIC2(talir, tag_control_freq_shift);
%include "talir/clock_recovery_mm_ext_ff.h"
GR_SWIG_BLOCK_MAGIC2(talir, clock_recovery_mm_ext_ff);
%include "talir/inject_msg_as_tag_cc.h"
GR_SWIG_BLOCK_MAGIC2(talir, inject_msg_as_tag_cc);
