require 'mkmf-rice'
require 'pkg-config'

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

PKGConfig.have_package('librsvg-2.0')
$LDFLAGS += ' '

create_makefile('keyer')

