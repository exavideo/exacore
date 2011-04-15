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
#have_library('rsvg-2')
#have_library('cairo')

pkg_config('librsvg-2.0')

create_makefile('keyer')

