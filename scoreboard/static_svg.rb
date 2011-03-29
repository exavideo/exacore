#!/usr/bin/env ruby 

template = File.open('test_files/bug.svg', 'r') do |f|
    f.read( )
end

while true
    # await request
    dummy = STDIN.read(1)
    if dummy.nil?
        break
    end

    size = [ template.length ].pack('L')

    STDOUT.write(size)
    STDOUT.write(template)
end
