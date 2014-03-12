#!/usr/bin/env ruby 

require 'patchbay'
require 'thin'

STDERR.puts "js_keyer_server.rb starting..."

Thin::Logging.silent = true

class JsKeyerControl < Patchbay
    put '/script' do
        script = put_data
        header = [ script.length, 0 ].pack('LL')
        STDOUT.write(header)
	STDOUT.write(script)
	STDOUT.flush

	render :json => ''
    end

protected
    def put_data
	inp = environment['rack.input']
	inp.rewind
	inp.read
    end
end

app = JsKeyerControl.new
app.run(:Host => '::1', :Port => 3004)

