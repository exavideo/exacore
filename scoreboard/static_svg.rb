#!/usr/bin/env ruby 

require 'patchbay'
require 'thin'

Thin::Logging.silent = true

template = File.open(ARGV[0], 'r') do |f|
    f.read( )
end

TRANS_TIME = 30
trans = 30
$trans_dir = 0

class StaticControl < Patchbay
    get '/bug_up' do
        $trans_dir = 1
        render :json => ''
    end

    get '/bug_down' do
        $trans_dir = -1
        render :json => ''
    end
end
app = StaticControl.new

Thread.new { app.run(:Host => '::', :Port => 3003) }

while true
    # await request
    dummy = STDIN.read(1)
    if dummy.nil?
        break
    end

    if $trans_dir == 1
        if trans < TRANS_TIME
            trans = trans + 1
        else
            $trans_dir = 0
        end
    elsif $trans_dir == -1
        if trans > 0
            trans = trans - 1
        else
            $trans_dir = 0
        end
    end
    
    size = [ template.length, (255 * trans) / TRANS_TIME, 0 ].pack('LCC')

    STDOUT.write(size)
    STDOUT.write(template)
end
