#include <facter/ruby/confine.hpp>
#include <facter/ruby/module.hpp>
#include <algorithm>

using namespace std;

namespace facter { namespace ruby {

    confine::confine(VALUE fact, VALUE expected, VALUE block) :
        _fact(fact),
        _expected(expected),
        _block(block)
    {
    }

    confine::confine(confine&& other)
    {
        *this = move(other);
    }

    confine& confine::operator=(confine&& other)
    {
        _fact = other._fact;
        _expected = other._expected;
        _block = other._block;
        return *this;
    }

    bool confine::suitable(module& facter) const
    {
        auto const& ruby = *api::instance();

        // If given a fact, either call the block or check the values
        if (!ruby.is_nil(_fact)) {
            volatile VALUE value = facter.normalize(facter.fact_value(_fact));
            if (ruby.is_nil(value)) {
                return false;
            }
            // Pass the value to the block if given one
            if (!ruby.is_nil(_block)) {
                volatile VALUE result = ruby.rb_funcall(_block, ruby.rb_intern("call"), 1, value);
                return !ruby.is_nil(result) && !ruby.is_false(result);
            }

            // Otherwise, if it's an array, search for the value
            if (ruby.is_array(_expected)) {
                bool found = false;
                ruby.array_for_each(_expected, [&](VALUE expected_value) {
                    expected_value = facter.normalize(expected_value);
                    found = ruby.equals(facter.normalize(expected_value), value);
                    return !found;
                });
                return found;
            }
            // Compare the value directly
            return ruby.case_equals(facter.normalize(_expected), value);
        }
        // If we have only a block, execute it
        if (!ruby.is_nil(_block)) {
            volatile VALUE result = ruby.rb_funcall(_block, ruby.rb_intern("call"), 0);
            return !ruby.is_nil(result) && !ruby.is_false(result);
        }
        return false;
    }

    void confine::mark() const
    {
        // Mark all VALUEs contained in the confine
        auto const& ruby = *api::instance();
        ruby.rb_gc_mark(_fact);
        ruby.rb_gc_mark(_expected);
        ruby.rb_gc_mark(_block);
    }

}}  // namespace facter::ruby
