%typemap(in) const StringList & {
    VALUE element;
    int i;
    int n; 

    $1 = new StringList;

    if (rb_check_array_type($input) == Qnil) {
        /* bomb out? */
    }

    n = RARRAY_LEN($input);
    for (i = 0; i < n; i++) {
        element = rb_ary_entry($input, i);
        fprintf(stderr, "%s\n", StringValueCStr(element));
        $1->push_back(StringValueCStr(element));
    }
}

%typemap(freearg) const StringList & {
    delete $1;
}
