/* -*-mode:c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#ifndef BOOL_ENCODER_HH
#define BOOL_ENCODER_HH

#include <vector>

#include "bool_decoder.hh"

/* libvpx lookup table to avoid the need for a loop in
 * BoolEncoder::put. Taken from libvpx/vp8/common/entropy.c
 */
const uint8_t vp8_norm[ 256 ] = {
    0, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* Routines taken from RFC 6386 */

class BoolEncoder
{
private:
  std::vector< uint8_t > output_ {};

  uint32_t range_ { 255 }, bottom_ { 0 };
  char bit_count_ { -24 };

  void add_one_to_output( void )
  {
    auto it = output_.end();
    while ( *--it == 255 ) {
      *it = 0;
      assert( it != output_.begin() );
    }
    ++*it;
  }

  void flush( void )
  {
    for ( uint8_t i = 0; i < 32; i++ ) {
      put( false ); /* try to match libvpx vp8_stop_encode(), not RFC 6386 */
    }
#if 0
    int c = bit_count_;
    uint32_t v = bottom_;

    if ( v & (1 << (32 - c)) ) {  /* propagate (unlikely) carry */
      add_one_to_output();
    }

    v <<= c & 7;               /* before shifting remaining output */
    c >>= 3;                   /* to top of internal buffer */
    while (--c >= 0) {
      v <<= 8;
    }
    c = 4;
    while (--c >= 0) {    /* write remaining data, possibly padded */
      output_.emplace_back( v >> 24 );
      v <<= 8;
    }
#endif
  }

public:
  BoolEncoder() 
  {
    output_.reserve( 1024 * 1024 ); //allocate a lot of space so we never have to realloc
  }

  void put( const bool value, const Probability probability = 128 )
  {
    uint32_t split = 1 + (((range_ - 1) * probability) >> 8);

    if ( value ) {
      bottom_ += split; /* move up bottom of interval */
      range_ -= split;  /* with corresponding decrease in range */
    } else {
      range_ = split;   /* decrease range, leaving bottom alone */
    }

    /* Taken from libvpx vp8/encoder/boolhuff.h */
    uint32_t shift = vp8_norm[ range_ ];

    range_ <<= shift;
    bit_count_ += shift;

    if ( bit_count_ >= 0 ) {
      int offset = shift - bit_count_;

      if ( ( bottom_ << (offset - 1)) & 0x80000000 ) {
        add_one_to_output();
      }

      output_.push_back( bottom_ >> ( 24 - offset ) );

      bottom_ <<= offset;
      shift = bit_count_;
      bottom_ &= 0xffffff;
      bit_count_ -= 8;
    }

    bottom_ <<= shift;
  }

  std::vector< uint8_t > finish( void )
  {
    flush();
    std::vector< uint8_t > ret( move( output_ ) );
    *this = BoolEncoder();
    return ret;
  }
};

#endif /* BOOL_ENCODER_HH */
