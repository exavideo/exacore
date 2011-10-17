#!/usr/bin/env ruby 

require 'rubygems'
require 'sinatra/base'
require 'thin'

Thin::Logging.silent = true

IN = 1
OUT = 2
UP = 3
DOWN = 4

$svgdata = ''
$trans_i = 0
$trans_nframes = 0
$trans_state = DOWN
$dirty_level = 0

# talk to stdio
Thread.new do
    begin
        while true
            # await request
            dummy = STDIN.read(1)
            if dummy.nil?
                break
            end

            size = [ $svgdata.length ].pack('L')

            alpha = 0

            # dissolve state logic
            Thread.exclusive do
                if $trans_state == UP
                    alpha = 255
                elsif $trans_state == DOWN
                    alpha = 0
                elsif $trans_state == IN 
                    alpha = ($trans_i.to_f / $trans_nframes.to_f * 255.0).to_i
                    $trans_i += 1
                    if $trans_i >= $trans_nframes
                        $trans_state = UP
                    end
                elsif $trans_state == OUT 
                    alpha = (255.0 - ($trans_i.to_f / $trans_nframes.to_f * 255.0)).to_i

                    $trans_i += 1
                    if $trans_i >= $trans_nframes
                        $trans_state = DOWN
                    end
                end
            end

            alphastr = [ alpha, $dirty_level ].pack('CC')

            STDOUT.write(size)
            STDOUT.write(alphastr)
            STDOUT.write($svgdata)
            STDOUT.flush
        end
    rescue Exception, e
        STDERR.puts "exception in IO thread"
        exit 1
    end
end

class KeyerServer < Sinatra::Base
    get '/' do
        # show some view
        erb :form
    end

    post '/key' do
        # read the postdata into template
        Thread.exclusive do
            $svgdata = request.env["rack.input"].read
        end
        STDERR.puts "key updated to #{$template}"
        204 # no content
    end

    put '/key' do
        # read the postdata into template
        Thread.exclusive do
            $svgdata = request.env["rack.input"].read
        end
        STDERR.puts "key updated to #{$template}"
        204 # no content
    end

    post '/dissolve_in/:frames' do
        Thread.exclusive do
            if $trans_state == DOWN
                $trans_nframes = params[:frames].to_i
                $trans_i = 0
                $trans_state = IN
                204 # no content
            else
                503 # service unavailable
            end
        end
    end

    post '/dirty_level/:n' do
        Thread.exclusive do
            $dirty_level = params[:n].to_ik
            204 # no content
        end
    end

    post '/dissolve_out/:frames' do
        Thread.exclusive do
            if $trans_state == UP
                $trans_nframes = params[:frames].to_i
                $trans_i = 0
                $trans_state = OUT
                204 # no content
            else
                503 # service unavailable
            end
        end
    end

    run!
end
