#!/usr/bin/env ruby

##
## Main keyer configuration.
## Instantiates the keyer channels, then runs the keyer.
##

###### Instantiate a KeyerApp object to configure and run ######
require './keyer.so'
include Keyer
keyer = KeyerApp.new

###### Scoreboard keyer channel - grabs SVGs from scoreboard program ######
scbd = SvgSubprocessCharacterGenerator.new(
    'cd /opt/scoreboard; ruby scoreboard_server.rb', 1
)
scbd.set_x 150
scbd.set_y 50
keyer.cg scbd

###### Static PNG keyer channel for 'normal' graphics ######
graphics = PngSubprocessCharacterGenerator.new('./http_keyer.rb -d 1')
graphics.set_x 0
graphics.set_y 0
keyer.cg graphics

###### JavaScript-driven compositor keyer for animated graphics ######
animated = JsCharacterGenerator.new('./js_keyer_server.rb')
animated.set_x 0
animated.set_y 0
keyer.cg animated

###### CEF keyer for HTML5 graphics (needs exacore-cef binary) ######
EXACORE_CEF="/home/armena/exacore-cef/build/exacore-cef/Release/exacore-cef"
CEF_URL="http://example.com"
cef = ShmCharacterGenerator.new("#{EXACORE_CEF} --url=#{CEF_URL}", 0)
cef.set_x 0
cef.set_y 0
keyer.cg cef

###### Bug keyer channel for static PNG bugs ######
bug = PngSubprocessCharacterGenerator.new('./http_keyer.rb -p 3005 -f /home/rpitv/bug.png')
bug.set_x 0
bug.set_y 0
keyer.cg bug

###### Input capture card configuration ######
inp = create_decklink_input_adapter_with_audio(1, 0, 0, RawFrame::CbYCrY8422, 8)
keyer.input inp

###### Output #1 (clean feed) capture card configuration ######
out1 = create_decklink_output_adapter_with_audio(2, 0, RawFrame::CbYCrY8422, 8)
keyer.output out1

###### Output #2 (main/dirty feed) capture card configuration ######
out2 = create_decklink_output_adapter_with_audio(0, 0, RawFrame::CbYCrY8422, 8)
keyer.output out2

###### Everything is now configured, so start the keyer's main loop ######
keyer.run
