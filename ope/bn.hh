#pragma once

#include <stdexcept>
#include <ostream>
#include <string>
#include <openssl/bn.h>
#include <openssl/crypto.h>

#include "errstream.hh"

namespace ope
{
    class _bignum_ctx
    {
        public:
        _bignum_ctx() : c(BN_CTX_new())
        {
          if (!c) throw std::runtime_error("BN_CTX_new failed");
        }
        
        ~_bignum_ctx() { BN_CTX_free(c); }
        
        BN_CTX *ctx() { return c; }
        
        static BN_CTX *the_ctx()
        {
          static _bignum_ctx cx;
          return cx.ctx();
        }
        
        private:
        BN_CTX *c;
    };
    
    class bignum
    {
        public:
        bignum() : b(BN_new())
        {
          throw_c(b);
        }
        
        explicit bignum(unsigned long v) : b(BN_new())
        {
          throw_c(b);
          throw_c(1 == BN_set_word(b, v));
        }
        
        bignum(const bignum &other) : b(BN_dup(other.b))
        {
          throw_c(b);
        }
        
        bignum(bignum &&other) noexcept: b(other.b)
        {
          other.b = nullptr;
        }
        
        bignum &operator=(const bignum &other)
        {
          if (this == &other) return *this;
          BIGNUM *nb = BN_dup(other.b);
          throw_c(nb);
          BN_free(b);
          b = nb;
          return *this;
        }
        
        bignum &operator=(bignum &&other) noexcept
        {
          if (this == &other) return *this;
          BN_free(b);
          b = other.b;
          other.b = nullptr;
          return *this;
        }
        
        bignum(const uint8_t *buf, size_t nbytes) : b(BN_new())
        {
          throw_c(b);
          throw_c(BN_bin2bn(buf, (int) nbytes, b));
        }
        
        explicit bignum(const std::string &v) : b(BN_new())
        {
          throw_c(b);
          throw_c(BN_bin2bn(reinterpret_cast<const unsigned char *>(v.data()), (int) v.size(), b));
        }
        
        ~bignum() { BN_free(b); }
        
        BIGNUM *bn() { return b; }
        
        const BIGNUM *bn() const { return b; }
        
        unsigned long word() const
        {
          unsigned long v = BN_get_word(b);
          if (v == 0xffffffffL)
          {
            throw std::runtime_error("out of range");
          }
          return v;
        }

#define op(opname, func, ...)                                        \
    bignum opname(const bignum& rhs) const {                          \
        bignum res;                                                   \
        throw_c(1 == func(res.bn(), b, rhs.bn(), ##__VA_ARGS__));      \
        return res;                                                   \
    }
        
        op(operator+, BN_add)
        
        op(operator-, BN_sub)
        
        // Note: BN_mod signature is (BIGNUM *r, const BIGNUM *a, const BIGNUM *m, BN_CTX *ctx)
        bignum operator%(const bignum &mod) const
        {
          bignum res;
          throw_c(1 == BN_mod(res.bn(), b, mod.bn(), _bignum_ctx::the_ctx()));
          return res;
        }
        
        bignum operator*(const bignum &rhs) const
        {
          bignum res;
          throw_c(1 == BN_mul(res.bn(), b, rhs.bn(), _bignum_ctx::the_ctx()));
          return res;
        }

#undef op

#define pred(predname, cmp)                 \
    bool predname(const bignum& other) const { \
        return BN_cmp(b, other.bn()) cmp;   \
    }
        
        pred(operator<, <  0)
        
        pred(operator<=, <= 0)
        
        pred(operator>, >  0)
        
        pred(operator>=, >= 0)
        
        pred(operator==, == 0)

#undef pred
        
        bignum invmod(const bignum &mod) const
        {
          bignum r;
          throw_c(BN_mod_inverse(r.bn(), b, mod.bn(), _bignum_ctx::the_ctx()));
          return r;
        }
        
        private:
        BIGNUM *b = nullptr;
    };
    
    static inline std::ostream &operator<<(std::ostream &out, const bignum &bn)
    {
      char *s = BN_bn2dec(bn.bn());
      if (!s) throw std::runtime_error("BN_bn2dec failed");
      out << s;
      OPENSSL_free(s);
      return out;
    }
}