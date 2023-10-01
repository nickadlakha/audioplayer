#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.71])
AC_INIT([audioplayer],[1.0],[nicklesh.adlakha@gmail.com])
AC_CONFIG_SRCDIR([audioplayer.c])
AC_CONFIG_HEADERS([config.h])

if test 1; then
CFLAGS="-std=gnu11 -Wall -O3"
fi

# Checks for programs.
AM_INIT_AUTOMAKE
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([avformat], [avformat_alloc_context, avformat_open_input, avformat_find_stream_info], [], [AC_MSG_ERROR(didn't find avformat library)], [])

AC_CHECK_LIB([avcodec], [avcodec_find_decoder, avcodec_alloc_context3, avcodec_parameters_to_context], [], [AC_MSG_ERROR(didn't find avcodec library)], [])

AC_CHECK_LIB([avutil], [av_frame_alloc], [], [AC_MSG_ERROR(didn't find avutil library)], [])

AC_CHECK_LIB([asound], [snd_pcm_open], [], [AC_MSG_ERROR(didn't find asound library)], [])
# Checks for header files.
AC_CHECK_HEADERS([unistd.h libavformat/avformat.h libavcodec/avcodec.h alsa/asoundlib.h linux/soundcard.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UINT8_T
# Checks for library functions.

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
