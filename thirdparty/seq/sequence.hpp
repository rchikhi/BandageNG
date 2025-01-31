//***************************************************************************
//* Copyright (c) 2015 Saint Petersburg State University
//* Copyright (c) 2011-2014 Saint Petersburg Academic University
//* All Rights Reserved
//* See file LICENSE for details.
//***************************************************************************

#pragma once

#include "log.hpp"
#include "nucl.hpp"
#include "utils/sfinae_checks.hpp"

#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <llvm/ADT/SparseBitVector.h>
#include <llvm/Support/TrailingObjects.h>

#include <vector>
#include <string>
#include <memory>
#include <sstream>
#include <cstring>

// Silence bogus gcc warnings
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"

class Sequence {
    // Type to store Seq in Sequences
    typedef uint64_t ST;
    // Number of bits in ST
    const static size_t STBits = sizeof(ST) << 3;
    // Number of nucleotides in ST
    const static size_t STN = (STBits >> 1);
    // Number of bits in STN (for faster div and mod)
    const static size_t STNBits = log_<STN, 2>::value;

    class ManagedNuclBuffer final : public llvm::ThreadSafeRefCountedBase<ManagedNuclBuffer>,
                                    protected llvm::TrailingObjects<ManagedNuclBuffer, ST> {
        friend TrailingObjects;

        ManagedNuclBuffer() {}

        ManagedNuclBuffer(size_t nucls, ST *buf) {
            std::uninitialized_copy(buf, buf + Sequence::DataSize(nucls), data());
        }

      public:
        void operator delete(void *p) { ::operator delete(p); }

        static ManagedNuclBuffer *create(size_t nucls) {
            void *mem = ::operator new(totalSizeToAlloc<ST>(Sequence::DataSize(nucls)));
            return new (mem) ManagedNuclBuffer();
        }

        static ManagedNuclBuffer *create(size_t nucls, ST *data) {
            void *mem = ::operator new(totalSizeToAlloc<ST>(Sequence::DataSize(nucls)));
            return new (mem) ManagedNuclBuffer(nucls, data);
        }

        const ST *data() const { return getTrailingObjects<ST>(); }
        ST *data() { return getTrailingObjects<ST>(); }

        std::unique_ptr<llvm::SparseBitVector<>> empty_nucls_ = nullptr;
    };

    size_t size_ : 32;
    size_t from_ : 31;
    bool   rtl_  : 1;  // Right to left + complimentary (?)
    llvm::IntrusiveRefCntPtr<ManagedNuclBuffer> data_;

    static size_t DataSize(size_t size) {
        return (size + STN - 1) >> STNBits;
    }

    template<typename S>
    void InitEmptyNucls(const S &s, bool rc = false) {
        int int_size = static_cast<int>(size_);
        int start = rc ? int_size - 1 : 0;
        int end = rc ? -1 : int_size;
        int step = rc ? -1 : 1;
        int cur_index = 0;

        for (int i = start; i != end; i += step) {
            if (LLVM_UNLIKELY(is_N(s[i]))) {
                if (LLVM_UNLIKELY(data_->empty_nucls_ == nullptr)) {
                    data_->empty_nucls_ = std::make_unique<llvm::SparseBitVector<>>();
                }
                data_->empty_nucls_->set(cur_index);
            }
            cur_index++;
        }
    }

    template<typename S>
    void InitFromNucls(const S &s, bool rc = false) {
        InitEmptyNucls(s, rc);

        size_t bytes_size = DataSize(size_);
        ST *bytes = data_->data();

        VERIFY(is_dignucl(s[0]) || is_nucl(s[0]) || is_N(s[0]));

        // Which symbols does our string contain : 0123 or ACGT?
        bool digit_str = is_dignucl(s[0]);

        // data -- one temporary variable corresponding to the i-th array element
        // and some counters
        ST data = 0;
        size_t cnt = 0;
        size_t cur = 0;

        if (rc) {
            for (int i = (int) size_ - 1; i >= 0; --i) {
                //VERIFY(is_dignucl(s[i]) || is_nucl(s[i]) || is_N(s[0]));
                char c = complement(digit_str ? s[(unsigned) i] : dignucl(s[(unsigned) i]));

                data = data | (ST(c) << cnt);
                cnt += 2;

                if (cnt == STBits) {
                    bytes[cur++] = data;
                    cnt = 0;
                    data = 0;
                }
            }
        } else {
            for (size_t i = 0; i < size_; ++i) {
                //VERIFY(is_dignucl(s[i]) || is_nucl(s[i]) || is_N(s[0]));
                char c = digit_str ? s[i] : dignucl(s[i]);

                data = data | (ST(c) << cnt);
                cnt += 2;

                if (cnt == STBits) {
                    bytes[cur++] = data;
                    cnt = 0;
                    data = 0;
                }
            }
        }

        if (cnt != 0)
            bytes[cur++] = data;

        for (; cur < bytes_size; ++cur)
            bytes[cur] = 0;
    }

    bool isEmptySymbol(size_t idx) const {
        if (LLVM_LIKELY(data_->empty_nucls_ == nullptr)) {
            return false;
        }
        return data_->empty_nucls_->test(idx);
    }

    char getNuclFromBuffer(size_t idx) const {
        const ST *bytes = data_->data();
        return static_cast<char>((bytes[idx >> STNBits] >> ((idx & (STN - size_t{1})) << size_t{1})) & size_t{3});
    }

    bool emptyNuclsEqual(const Sequence &that) const {
        return data_->empty_nucls_ == that.data_->empty_nucls_
            || (data_->empty_nucls_ != nullptr
                && that.data_->empty_nucls_ != nullptr
                && *data_->empty_nucls_ == *that.data_->empty_nucls_);
    }

    //Low level constructor. Handle with care.
    Sequence(const Sequence &seq, size_t from, size_t size, bool rtl)
            : size_(size), from_(from), rtl_(rtl), data_(seq.data_) {}

public:
    explicit Sequence(size_t size, bool allNs = false)
            : size_(size), from_(0), rtl_(false), data_(ManagedNuclBuffer::create(size_)) {
        if (allNs) {
            data_->empty_nucls_ = std::make_unique<llvm::SparseBitVector<>>();
            for (size_t i = from_; i < size_; ++i)
                data_->empty_nucls_->set(i);
        }
    }

    /**
     * Sequence initialization (arbitrary size string)
     *
     * @param s ACGT or 0123-string
     */
    explicit Sequence(const char *s, bool rc = false)
            : Sequence(strlen(s)) {
        InitFromNucls(s, rc);
    }

    explicit Sequence(char *s, bool rc = false)
            : Sequence(strlen(s)) {
        InitFromNucls(s, rc);
    }

    template<typename S, typename = std::enable_if_t<has_size_method<S>::value>>
    explicit Sequence(const S &s, bool rc = false)
            : Sequence(s.size()) {
        if (s.size() > 0) {
            InitFromNucls(s, rc);
        }
    }

    Sequence()
            : Sequence(size_t{0}) {
        memset(data_->data(), 0, DataSize(size_));
    }

    Sequence(const Sequence &s)
            : Sequence(s, s.from_, s.size_, s.rtl_) {}

    Sequence(Sequence &&) = default;

    Sequence &operator=(const Sequence &rhs) {
        if (&rhs == this)
            return *this;

        from_ = rhs.from_;
        size_ = rhs.size_;
        rtl_ = rhs.rtl_;
        data_ = rhs.data_;

        return *this;
    }

    Sequence &operator=(Sequence &&) = default;

    char operator[](const size_t index) const {
        VERIFY_DEV(index < size_);
        if (rtl_) {
            size_t i = from_ + size_ - 1 - index;
            if (LLVM_UNLIKELY(isEmptySymbol(i))) {
                return 'N';
            }
            return nucl(complement(getNuclFromBuffer(i)));
        } else {
            size_t i = from_ + index;
            if (LLVM_UNLIKELY(isEmptySymbol(i))) {
                return 'N';
            }
            return nucl(getNuclFromBuffer(i));
        }
    }

    bool operator==(const Sequence &that) const {
        if (size_ != that.size_)
            return false;

        if (data_ == that.data_ && from_ == that.from_ && rtl_ == that.rtl_ && emptyNuclsEqual(that))
            return true;

        for (size_t i = 0; i < size_; ++i) {
            if (this->operator[](i) != that[i]) {
                return false;
            }
        }
        return true;
    }

    bool operator!=(const Sequence &that) const {
        return !(operator==(that));
    }

    /**
     * @todo Might be optimized via int comparison (not so easy)
     */
    bool operator<(const Sequence &that) const {
        size_t s = std::min(size_, that.size_);
        for (size_t i = 0; i < s; ++i) {
            if (this->operator[](i) != that[i]) {
                return (this->operator[](i) < that[i]);
            }
        }
        return (size_ < that.size_);
    }

    Sequence GetReverseComplement() const {
        return {*this, from_, size_, !rtl_};
    }

    inline Sequence operator<<(char c) const;

    /**
     * @param from inclusive
     * @param to exclusive;
     */
    inline Sequence Subseq(size_t from, size_t to) const;

    inline Sequence Subseq(size_t from) const; // up to size_ by default

    inline Sequence operator+(const Sequence &s) const;

    inline Sequence First(size_t count) const;

    inline Sequence Last(size_t count) const;

    inline size_t find(const Sequence &t, size_t from = 0) const;

    template<class Seq>
    Seq start(size_t k) const;

    template<class Seq>
    Seq end(size_t k) const;

    inline std::string str() const;

    inline std::string err() const;

    size_t size() const {
        return size_;
    }

    size_t capacity() const {
        return DataSize(size_) * sizeof(ST);
    }

    bool empty() const {
        return size() == 0;
    }

    bool missing() const {
        // No N's - nothing is missed
        if (!data_->empty_nucls_)
            return false;

        // All N's are set
        return size_ == data_->empty_nucls_->count();
    }

    template<class Seq>
    bool contains(const Seq& s, size_t offset = 0) const {
        VERIFY_DEV(offset + s.size() <= size());

        for (size_t i = 0, e = s.size(); i != e; ++i)
            if (operator[](offset + i) != s[i])
                return false;

        return true;
    }
};

inline std::ostream &operator<<(std::ostream &os, const Sequence &s);

template<class Seq>
Seq Sequence::start(size_t k) const {
    return Seq(unsigned(k), *this);
}

template<class Seq>
Seq Sequence::end(size_t k) const {
    return Seq(unsigned(k), *this, size_ - k);
}

// O(1)
//including from, excluding to
//safe if not #DEFINE NDEBUG
Sequence Sequence::Subseq(size_t from, size_t to) const {
    VERIFY(to >= from);
    VERIFY(to <= size_);
    //VERIFY(to - from <= size_);
    if (rtl_) {
        return {*this, from_ + size_ - to, to - from, true};
    } else {
        return {*this, from_ + from, to - from, false};
    }
}

//including from, excluding to
Sequence Sequence::Subseq(size_t from) const {
    return Subseq(from, size_);
}

Sequence Sequence::First(size_t count) const {
    return Subseq(0, count);
}

Sequence Sequence::Last(size_t count) const {
    return Subseq(size_ - count);
}

/**
 * @todo : must be KMP or hashing instead of this
 */
size_t Sequence::find(const Sequence &t, size_t from) const {
    for (size_t i = from; i <= size() - t.size(); i++) {
        if (Subseq(i, i + t.size()) == t) {
            return i;
        }
    }
    return -1ULL;
}

/**
 * @todo optimize
 */
Sequence Sequence::operator+(const Sequence &s) const {
    return Sequence(str() + s.str());
    // TODO might be opposite to correct
    //    int total = size_ + s.size_;
    //    std::vector<Seq<4> > bytes((total + 3) >> 2);
    //    for (size_t i = 0; i < size_; ++i) {
    //        bytes[i / 4] = (bytes[i / 4] << operator [](i)); // TODO :-) use <<=
    //    }
    //    for (size_t i = 0, j = size_; i < s.size_; ++i, ++j) {
    //        bytes[j / 4] = (bytes[j / 4]) << s[i];
    //    }
    //    return Sequence(new Data(bytes), 0, total, false);
}

std::string Sequence::str() const {
    std::string res(size_, '-');
    for (size_t i = 0; i < size_; ++i) {
        res[i] = nucl(this->operator[](i));
    }
    return res;
}

std::string Sequence::err() const {
    std::ostringstream oss;
    oss << "{ *data=" << data_->data() <<
            ", from_=" << from_ <<
            ", size_=" << size_ <<
            ", rtl_=" << int(rtl_) <<
            ", empty_nucls_=" << data_->empty_nucls_.get() << " }";
    return oss.str();
}

std::ostream &operator<<(std::ostream &os, const Sequence &s) {
    os << s.str();
    return os;
}

#pragma GCC diagnostic pop
