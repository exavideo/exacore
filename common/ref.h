/*
 * Copyright 2011 Andrew H. Armenia.
 * 
 * This file is part of openreplay.
 * 
 * openreplay is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * openreplay is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with openreplay.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _OPENREPLAY_REF_H
#define _OPENREPLAY_REF_H

template <class T>
class ref { /* lowercased to emphasize how commonplace it should be */
    public:
        ref( ) {
            /* initialize to a null state */
            _impl = NULL;
        }

        virtual ~ref( ) {
            /* decrement refcount on the implementation */
            if (_impl != NULL) {
                _impl->decref( );
            }
        }

        ref(T* ptr) {
            _impl = new _ref_impl;
            _impl->refcnt = 1;
            _impl->ptr = ptr;
        }

        ref(const ref& src) {
            /* copy impl. */
            _impl = src->_impl;

            /* increment reference counter */
            _impl->incref( );
        }

        const ref& operator=(const ref& rhs) {
            if (_impl != NULL) {
                _impl->decref( );
                _impl = NULL;
            }

            _impl = rhs->_impl;
            _impl->incref( );
        }

        const T& operator->() const {
            if (_impl == NULL) {
                throw std::runtime_error("null dereference"); 
            }

            return *(_impl->ptr);
        }

        T& operator->() {
            if (_impl == NULL) {
                throw std::runtime_error("null dereference"); 
            }

            return *(_impl->ptr);
        }

        bool is_null( ) {
            if (_impl == NULL) {
                return false;
            } else {
                return true;
            }
        }

        /* Is this thread-safe? Is it efficient? Right now, NO! */
        ref<T> make_unique( ) {
            if (_impl->refcnt == 1) {
                /* already unique so just return a copy. */
                return *this;
            } else {
                /* not unique... make a new copy */
                return ref<T>(new T(*(_impl->ptr)));
            }
        }

    protected:
        struct _ref_impl {
            T *ptr;
            unsigned int refcnt;
            _ref_impl( ) { refcnt = 0; }
            void incref( ) {
                /* don't care about the return */
                __sync_add_and_fetch(&refcnt, 1); 
            }
            
            /* 
             * Once decref( ) is called on a _ref_impl, the pointer to the
             * _ref_impl should be discarded. IMMEDIATELY. This is important
             * because the object deletes itself!
             */
            void decref( ) {
                int new_rc;
                new_rc = __sync_sub_and_fetch(&refcnt, 1);
                if (new_rc == 0) {
                    delete ptr;
                    delete this;
                }
            }
        } *_impl;
};

#if 0

template <class T>
class array_ref : public ref<T> {
    public:
        array_ref( ) : ref<T>( ) { }
        array_ref::~array_ref( ) { }
        array_ref(const array_ref& src) : ref<T>(src) { }
        
        const array_ref& operator=(const array_ref& rhs) {
            assign(rhs->_impl);
            return *this;
        }

        const T& operator[](int i) const {
            
        }

        T& operator[](int i) const {
            
        }
}

#endif

#endif
