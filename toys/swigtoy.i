%module swigtoy
%{
    #include <string>
    class MyClass {
        public:
        virtual const char *hello( ) {
            return "Hello world!";
        }

        virtual const char *something_else( ) {
            return "Blah";
        }

        virtual const char *goodbye( ) {
            return "That's all folks";
        }

        void blech(std::string &out) {
            out = "blech";
        }

        void blargh(std::string &out) {
            out.assign("abc\0def\0gh", 10);
        }
    };
%}

%include "typemaps.i"
%include "std_string.i"

class MyClass {
    public:
        /* question: does this work even though the interfaces don't match? */
        virtual const char *hello( );
        virtual const char *goodbye( );

        void blech(std::string &OUTPUT);
        void blargh(std::string &OUTPUT);
};
