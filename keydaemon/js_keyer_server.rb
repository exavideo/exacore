#!/usr/bin/env ruby 

require 'patchbay'
require 'thin'

STDERR.puts "js_keyer_server.rb starting..."

Thin::Logging.silent = true

class JsKeyerControl < Patchbay
    put '/script' do
        write_message(0, put_data)
        render :json => '{ status: \'ok\' }'
    end

    put '/command' do
        write_message(1, put_data)
        render :json => '{ status: \'ok\' }'
    end

protected
    def write_message(type, msg)
        header = [ msg.length, type ].pack('LL')
        STDOUT.write(header)
        STDOUT.write(msg)
        STDOUT.flush
    end

    def put_data
        inp = environment['rack.input']
        inp.rewind
        inp.read
    end
end

app = JsKeyerControl.new
app.run(:Host => '::1', :Port => 3004)

