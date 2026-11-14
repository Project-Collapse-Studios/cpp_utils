#pragma once
#include <string>
#include <unordered_set>
#include <cstdint>
#include <memory>
#include <ostream>
#include "cutil_string.h"
#include "endless_well.h"

namespace cutil {
    class string_reference_manager;

    /// EXCEPTIONS
    class string_reference_manager_exception : public std::exception
    {
    public:
        [[nodiscard]] const char * what() const noexcept override
        {
            return "Cannot create a new string reference without a valid manager!";
        }
    };

    class except_strref_invalid : public std::exception
    {
    public:
        [[nodiscard]] const char * what() const noexcept override
        {
            return "String reference is invalid! (Hasn't been properly initialized)";
        }
    };

    /// String reference is a new string type intended to represent data that often repeats, such as the original reason - KV1 formats in the Source Engine
    /// This class is merely a pointer wrapper, it represents a string existing in the manager.
    /// Creating it using the default constructor will make it an invalid string reference (empty string, no write access). You can then use the assignment operator to overwrite it with a valid one.
    class string_reference {

        /// A representation of this string_reference that lives inside the set
        class str_ref_repr {
        public:
            string_reference_manager* manager = nullptr; // The manager we belong to
            size_t start = 0; // The starting character's index belonging to our string
            std::uint16_t len = 0; // Length of the string

            /// Stores the amount of string_reference that reference us.
            size_t reference_count = 0;
        };

        str_ref_repr* our_representation = nullptr;

        explicit string_reference(str_ref_repr* reference) {
            this->our_representation = reference;
            if (!this->isValid()) {
                throw except_strref_invalid();
            }
            ++this->our_representation->reference_count; // We start to reference it
        };
    public:
        /// Default constructor, creates an invalid string reference meant to be overwritten later.
        string_reference() = default;

        string_reference(const string_reference& other) { // Copy constructor
            if (!other.isValid()) {
                throw except_strref_invalid();
            }
            this->our_representation = other.our_representation;
            ++this->our_representation->reference_count;
        }

        string_reference(string_reference&& other) noexcept {
            this->our_representation = other.our_representation;
        }

        ~string_reference() {
            if (this->our_representation != nullptr) {
                --this->our_representation->reference_count;
            }
        }



        friend string_reference_manager;

        // BEGIN PUBLIC INTERFACE
    public:
        // Conversions
        operator std::string() const;

        operator std::string_view() const
        {
            return {this->begin(), this->end()};
        }

        // Operators

        std::string getString() const;

        /// Assign other string_reference, may change the manager we belong to!
        string_reference& operator=(const string_reference& other) {
            if (!other.isValid()) {
                throw except_strref_invalid();
            }
            if (this->isValid()) {
                --this->our_representation->reference_count;
            }
            this->our_representation = other.our_representation;
            ++this->our_representation->reference_count;
            return *this;
        }

        /// Assign a string (creates a new string_reference), assigned to the same manager.
        string_reference& operator=(const std::string& other);
        string_reference& operator=(const std::string_view& other);
        string_reference& operator=(const char* other);


        std::string::const_iterator begin() const;

        std::string::const_iterator end() const {
            return this->begin() + this->our_representation->len;
        }

        [[nodiscard]] bool isValid() const {
            if (this->our_representation == nullptr) {
                return false;
            }
            if (this->our_representation->manager == nullptr) {
                return false;
            }
            return true;
        }


        /// Compare two string references, compares only by pointer!
        /// Use str_compare to actually compare the underlaying strings
        bool operator==(const string_reference& other) const {
            return this->our_representation == other.our_representation;
        }


        /// String compare, with automatic caseless detection (based on the manager's settings).
        bool operator==(const std::string_view& str) const;

        bool str_compare(const std::string_view other) const {
            return std::string_view(*this) == other;
        }

        bool str_compare_caseless(const std::string_view other) const {
            return iequals(std::string_view(*this), other);
        }



        bool starts_with(const std::string_view sv) const noexcept {
            if (sv.length() > this->our_representation->len) { return false; }
            size_t indx = 0;
            for (const auto c : *this) {
                if (c != sv[indx++]) {
                    return false;
                }
            }
            return true;
        }

        bool starts_with(const char c) const noexcept {
            if (this->our_representation->len == 0) {return false;}
            return *this->begin() == c;
        }

        bool starts_with(const char* chararr) const {
            const auto begin = this->begin();
            for (size_t i = 0; chararr[i] != '\0' && i < this->our_representation->len; ++i) {
                if (*(begin + i) != chararr[i]) {return false;}
            }
            return true;
        }

        std::string substr(size_t start = 0, size_t length = std::string::npos) const;

        std::uint16_t length() const {
            return this->our_representation->len;
        }

        bool empty() const {
            return this->our_representation->len == 0;
        }

        friend std::ostream& operator<<(std::ostream& op, const string_reference& obj) {
            for (const char c : obj) {
                op << c;
            }
            return op;
        }

    };

    template<typename V>
    concept StringAlike =   std::same_as<V, std::string> ||
                            std::same_as<V, std::string_view> ||
                            std::same_as<V, string_reference> ||
                           (std::is_array_v<V> && std::same_as<std::remove_extent_t<V>, char>) ||
                            //std::same_as<V, char*> ||
                            std::same_as<V, const char*>;


    class string_reference_manager {

        /// Used for hashing
        struct str_ref_hash {
            using is_transparent = std::true_type;

            /// Hashing function, custom made to only read the buffer and not compare the actual object
            std::size_t operator()(const string_reference::str_ref_repr* ref) const noexcept;
            std::size_t operator()(const std::string& ref) const noexcept;
            std::size_t operator()(std::string_view ref) const noexcept;
            std::size_t operator()(const char* ref) const noexcept;
        };

        struct str_ref_comp
        {
            using is_transparent = std::true_type;

            constexpr bool operator()(const std::string& ref, const string_reference::str_ref_repr* ref2) const
            {
                return std::string_view { ref2->manager->buffer.begin() + ref2->start, ref2->manager->buffer.begin() + ref2->start + ref2->len } == ref;
            };
            constexpr bool operator()(const std::string_view ref, const string_reference::str_ref_repr* ref2) const
            {
                return std::string_view { ref2->manager->buffer.begin() + ref2->start, ref2->manager->buffer.begin() + ref2->start + ref2->len } == ref;
            };
            constexpr bool operator()(const string_reference::str_ref_repr* ref, const string_reference::str_ref_repr* ref2) const
            {
                return std::string_view{ ref->manager->buffer.begin() + ref->start, ref->manager->buffer.begin() + ref->start + ref->len }
                == std::string_view { ref2->manager->buffer.begin() + ref2->start, ref2->manager->buffer.begin() + ref2->start + ref2->len };
            };
            constexpr bool operator()(const char* ref, const string_reference::str_ref_repr* ref2) const {
                return std::string_view{ref} == std::string_view { ref2->manager->buffer.begin() + ref2->start, ref2->manager->buffer.begin() + ref2->start + ref2->len };
            }
        };

        friend string_reference;
        friend string_reference::str_ref_repr;

        std::unordered_set<string_reference::str_ref_repr*, str_ref_hash, str_ref_comp> set;
        /// Manager has exclusive ownership of buffer. No operations from outside are allowed.
        std::string buffer{};
        EndlessWell<string_reference::str_ref_repr> allocator{};

        string_reference::str_ref_repr empty_repr;
        bool caseless = false;

    public:
        /// Create the manager.
        explicit string_reference_manager(bool caseless_ = false) {
            this->empty_repr.len = 0;
            this->empty_repr.start = 0;
            this->empty_repr.manager = this;
            this->caseless = caseless_;
        }

        /// Create whilst passing a ready to be read buffer.
        explicit string_reference_manager(std::string_view buffer, bool caseless_ = false) {
            this->buffer = buffer;
            this->empty_repr.len = 0;
            this->empty_repr.start = 0;
            this->empty_repr.manager = this;
            this->caseless = caseless_;
        }

        ~string_reference_manager() = default;

        /// Checks if the manager and its objects differentiate between cased and caseless chars.
        [[nodiscard]] bool isCaseless() const {
            return this->caseless;
        }

        /// Request a string reference from us.
        template<StringAlike V>
        [[nodiscard]] string_reference request(const V& str) {

            // HANDLE EMPTY STRINGS
            if constexpr (std::is_array_v<V> || std::same_as<V, const char*>) {
                if (str[0] == '\0') {
                    return string_reference{&this->empty_repr};
                }
            } else {
                if (str.empty()) {
                    return string_reference{&this->empty_repr};
                }
            }

            // CALCULATE LENGTH OF STRREF
            size_t ref_len = 0;
            if constexpr (std::is_array_v<V> || std::same_as<V, const char*>) {
                for (size_t i = 0; str[i] != '\0'; ++i) {
                    ++ref_len;
                }
            } else {
                ref_len = str.length();
            }

            // FIND IF STRREF ALREADY EXISTS
            // Our hash and compare set supports all types of V
            std::unordered_set<string_reference::str_ref_repr*, str_ref_hash, str_ref_comp>::const_iterator it;
            if (!this->caseless) {
                it = this->set.find(str);
            } else {
                // Is there a better way to do this?
                std::string A;
                A.reserve(ref_len);
                if constexpr (std::is_array_v<V> || std::same_as<V, const char*>) {
                    for (size_t i = 0; str[i] != '\0'; ++i) {
                        A += std::tolower(str[i]);
                    }
                } else {
                    for (const auto ch : str) {
                        A += std::tolower(ch);
                    }
                }
                it = this->set.find(A);
            }

            /// CREATE NEW STRREF - DOESN'T EXIST
            if (it == this->set.end()) {
                //auto out = new string_reference::str_ref_repr{};
                auto out = this->allocator.get_element();
                out->len = ref_len;
                out->manager = this;
                out->start = this->buffer.length();
                this->buffer.append(str); // This should automatically handle conversion to a supported type.
                this->set.insert(out);
                return string_reference{out};
            }
            // String already exists, delete the previous object and grab the one from the set
            //delete out;
            return string_reference{*it};

        }

        /// Clears unused references from memory.
        /// This is expensive!
        /// This doesn't actually delete the references themselves!
        void ClearUnusedReferences() {
            for (auto it = this->set.begin(); it != this->set.end(); ++it) {
                if ((*it)->reference_count == 0) {
                    this->buffer.erase((*it)->start, (*it)->len);
                    this->set.erase(it);
                }
            }
        }
    };
}