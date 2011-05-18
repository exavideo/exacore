require 'sinatra/base'
$svg = 'Hello World'

Thread.new do
    while true
        # await request
        dummy = STDIN.read(1)
        if dummy.nil?
            break
        end

        svg = Thread.exclusive { $svg.dup }
        size = [ svg.length ].pack('L')

        STDOUT.write(size)
        STDOUT.write(svg)
    end
end

class APIServer < Sinatra::Base
    post '/template' do
        request.body.rewind
        svg = request.body.read
        Thread.exclusive { $svg = svg }
    end
end

APIServer.run!
