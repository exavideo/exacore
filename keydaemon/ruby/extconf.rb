require 'mkmf-rice'
dir_config('keyer', 
    [
        '..',
        '../../common',
        '../../drivers', 
        '../../raw_frame',
        '../../thread',
        '../../graphics'
    ], [
        '..'
    ]
)

have_library('keyerfuncs')
create_makefile('keyer')

