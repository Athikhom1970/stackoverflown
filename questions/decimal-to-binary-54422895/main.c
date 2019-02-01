// https://github.com/KubaO/stackoverflown/tree/master/questions/decimal-to-binary-54422895
#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Bytes Class Interface

typedef uint8_t *Bytes;
typedef const uint8_t *cBytes;

Bytes new_Bytes(size_t size);
size_t Bytes_size(cBytes bytes);
void Bytes_truncate(Bytes bytes, size_t new_size);
void free_Bytes(cBytes bytes);
char *Bytes_to_hex(cBytes bytes);

static inline void Bytes_set_bit(Bytes bytes, size_t bit_num) {
   bytes[bit_num / 8] |= 1 << (bit_num % 8);
}

// Division and Base Conversion Interface

typedef enum {
   REMAINDER = 1,           /* there is a non-zero remainder */
   ZERO = 2,                /* the quotient is zero or null */
   NULL_DECIMAL = 4,        /* the dividend is null or empty */
   NON_DECIMALS = 8,        /* division was terminated on non-decimal characters */
   LEADING_ZERO_COUNT = 16, /* count of leading zeroes in the quotient */
   LEADING_ZERO_COUNT_MASK = ~(LEADING_ZERO_COUNT - 1),
   CLR_CARRY_MASK = ~REMAINDER,
   CLR_ZERO_MASK = ~ZERO,
} DivFlags;

DivFlags divideBy2(char *decimal);
Bytes base10to256(const char *decimal);

// Division and Base Conversion Implementation

DivFlags divideBy2(char *decimal) {
   DivFlags flags = ZERO;
   if (!decimal) return flags | NULL_DECIMAL;
   char c;
   while ((c = *decimal)) {
      if (c < '0' || c > '9') return flags | NON_DECIMALS;
      c = c - '0' + ((flags & REMAINDER) ? 10 : 0);
      if (c & 1)
         flags |= REMAINDER;
      else
         flags &= CLR_CARRY_MASK;
      c >>= 1;
      assert(c >= 0 && c <= 9);
      if (c)
         flags &= CLR_ZERO_MASK;
      else if (flags & ZERO)
         if ((flags & LEADING_ZERO_COUNT_MASK) != LEADING_ZERO_COUNT_MASK)
            flags += LEADING_ZERO_COUNT;
      *decimal++ = c + '0';
   }
   return flags;
}

static void base10to256_impl(Bytes const bytes, char *decimal, size_t len);

Bytes base10to256(const char *decimal) {
   assert(decimal);
   size_t const dec_len = strlen(decimal);
   char *const dec_buf = malloc(dec_len + 1);
   if (!dec_buf) return NULL;
   memcpy(dec_buf, decimal, dec_len + 1);

   const size_t BASE_RATIO_NUM = 416, /* ceil(log(10)/log(256)*1000) */
       BASE_RATIO_DENOM = 1000;
   assert(dec_len <= (SIZE_MAX / BASE_RATIO_NUM));
   size_t const len = (size_t)(dec_len * BASE_RATIO_NUM / BASE_RATIO_DENOM) + 1;
   Bytes const bytes = new_Bytes(len);  // little-endian
   if (bytes) base10to256_impl(bytes, dec_buf, len);
   free(dec_buf);
   return bytes;
}

static void base10to256_impl(Bytes const bytes, char *decimal, size_t const len) {
   for (size_t bit_num = 0;; bit_num++) {
      DivFlags const flags = divideBy2(decimal);
      assert(!(flags & NULL_DECIMAL));
      decimal += flags / LEADING_ZERO_COUNT;
      if (flags & ZERO && !(flags & REMAINDER)) {
         size_t new_len = ((bit_num + 7) / 8);
         if (!new_len) {  // we leave always one byte in the array
            bytes[0] = 0;
            ++new_len;
         }
         Bytes_truncate(bytes, new_len);
         break;
      }
      // here, there are still non-zero bits - in the dec[imal] and/or in the carry
      assert((bit_num / 8) < len);
      if (flags & REMAINDER) Bytes_set_bit(bytes, bit_num);
   }
}

// Tests

void check_bytes(const char *decimal, const char *bytes_expected, size_t bytes_len,
                 const char *hex_expected) {
   cBytes bytes = base10to256(decimal);
   assert(bytes && Bytes_size(bytes) == bytes_len);
   assert(memcmp(bytes, bytes_expected, bytes_len) == 0);
   char *hex = Bytes_to_hex(bytes);
   assert(hex && strcmp(hex, hex_expected) == 0);
   printf("%s\n", hex);
   free(hex);
   free_Bytes(bytes);
}

int main() {
   check_bytes("4294967297" /* 2^32+1 */, "\1\0\0\0\1", 5, "01 00000001");
   check_bytes("4294967296" /* 2^32   */, "\0\0\0\0\1", 5, "01 00000000");
   check_bytes("4294967295" /* 2^32-1 */, "\xFF\xFF\xFF\xFF", 4, "FFFFFFFF");
   check_bytes("16777217" /* 2^24+1 */, "\1\0\0\1", 4, "01000001");
   check_bytes("16777216" /* 2^24   */, "\0\0\0\1", 4, "01000000");
   check_bytes("16777215" /* 2^24-1 */, "\xFF\xFF\xFF", 3, "FFFFFF");
   check_bytes("256", "\0\1", 2, "0100");
   check_bytes("255", "\xFF", 1, "FF");
   check_bytes("254", "\xFE", 1, "FE");
   check_bytes("253", "\xFD", 1, "FD");
   check_bytes("3", "\3", 1, "03");
   check_bytes("2", "\2", 1, "02");
   check_bytes("1", "\1", 1, "01");
   check_bytes("0", "\0", 1, "00");
}

// Bytes Implementation

struct BytesImpl {
   size_t size;
   uint8_t data[1];
};
static const size_t Bytes_header_size = offsetof(struct BytesImpl, data);

Bytes new_Bytes(size_t size) {
   _Static_assert(offsetof(struct BytesImpl, data) == sizeof(size_t),
                  "unexpected layout of struct BytesImpl");
   assert(size <= SIZE_MAX - Bytes_header_size);
   struct BytesImpl *impl = calloc(Bytes_header_size + size, 1);
   if (!impl) return NULL;
   impl->size = size;
   return &impl->data[0];
}

static const struct BytesImpl *Bytes_get_const_impl_(cBytes bytes) {
   return (const struct BytesImpl *)(const void *)((const char *)bytes -
                                                   Bytes_header_size);
}

static struct BytesImpl *Bytes_get_impl_(Bytes bytes) {
   return (struct BytesImpl *)(void *)((char *)bytes - Bytes_header_size);
}

size_t Bytes_size(cBytes bytes) { return Bytes_get_const_impl_(bytes)->size; }

void Bytes_truncate(Bytes bytes, size_t new_size) {
   size_t *size = &Bytes_get_impl_(bytes)->size;
   if (*size <= new_size) return;
   *size = new_size;
}

void free_Bytes(cBytes bytes) {
   if (bytes) free((void *)(intptr_t)(const void *)Bytes_get_const_impl_(bytes));
}

char *Bytes_to_hex(cBytes bytes) {
   size_t n = Bytes_size(bytes);
   size_t spaces = (n - 1) / 4;
   char *const out = malloc(n * 2 + spaces + 1);
   if (out)
      for (char *o = out; n;) {
         uint8_t const c = bytes[n - 1];
         snprintf(o, 3, "%02" PRIX8, c);
         o += 2;
         n--;
         if (n && n % 4 == 0) {
            assert(spaces);
            *o++ = ' ';
            spaces--;
         }
      }
   return out;
}
