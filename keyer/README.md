# Exacore Keyer
Multi-channel keyer with support for various types of still 
and animated graphics.

## Building
From the root of your exacore tree, run `make keyer/keyer.so`.

## Basic Usage
The keyer is configured using a ruby script. Here is the simplest possible one:

```
# Create an instance of the keyer app.
require './keyer.so'
include Keyer
keyer = KeyerApp.new

# open decklink capture card 0 for input and attach to the keyer
in_card = create_decklink_input_adapter_with_audio(0, 0, 0, RawFrame::CbYCrY8422, 8)
keyer.input in_card

# open decklink capture card 1 for output and attach to the keyer
out_card = create_decklink_output_adapter_with_audio(1, 0, RawFrame::CbYCrY8422, 8)
keyer.output out_card

# start the keyer
keyer.run
```

Save this as `demo_keyer.rb` in this directory, then run `ruby demo_keyer.rb`.
This should copy video from the input card to the output card.

## Adding a Channel
Let's add a PNG keyer. We'll add the following to our config file:

```
# Create an instance of the keyer app.
require './keyer.so'
include Keyer
keyer = KeyerApp.new
```
```
# Create a PNG keyer channel.
png_cg = PngSubprocessCharacterGenerator.new(
	'./http_keyer.rb -p 3005 -f /path/to/some_image.png'
)
png_cg.set_x 960
png_cg.set_y 540
keyer.cg png_cg
```
```
# open decklink capture card 0 for input and attach to the keyer
in_card = create_decklink_input_adapter_with_audio(0, 0, 0, RawFrame::CbYCrY8422, 8)
keyer.input in_card

# open decklink capture card 1 for output and attach to the keyer
out_card = create_decklink_output_adapter_with_audio(1, 0, RawFrame::CbYCrY8422, 8)
keyer.output out_card

# start the keyer
keyer.run
```

Start your keyer, then navigate to `http://localhost:3005/upload.html`. Try
uploading a different image, or play around with the transition buttons.

### Under the Hood
This starts a *subprocess* `http_keyer.rb` to load the PNG image and pass it 
to the keyer channel. As the name implies, `http_keyer.rb` can be controlled
over HTTP: it provides a simple API for external apps to control the keyer
channel. The interface between the keyer and subprocesses is defined in
`subprocess_character_generator.cpp`. In a nutshell it's a pair of pipes: one
is used by the keyer to request frames from the subprocess, and the other is
used by the subprocess to send frame data.

## Clean and Dirty Feeds
It's possible to attach multiple capture cards to the keyer as outputs. In
this case, the graphics will be keyed in in order to their *dirty level*.
All graphics of dirty level zero will be keyed in, then the frame will be
sent out the first card defined. Then all graphics of dirty level one get
keyed in, and the frame is sent out the second card, and so on.

For the subprocess-type keyers, the dirty level is defined by the subprocess.
For `http_keyer.rb` this can be passed in with the `-d` command line flag
e.g. `-d 0`, `-d 1`.

## Subprocess Interface Details
Subprocesses interface to the keyer via stdin and stdout redirection. If 
you're writing a subprocess plugin for the keyer, you will need to wait
for messages on stdin, then write some data on stdout.

Whenever a byte is received on stdin, the subprocess should generate a frame,
or at least a frame header, on stdout in response. The contents of the byte 
from stdin don't matter - it's just a signal to generate a frame. Each frame 
is preceded by three header fields: a size (in bytes) of the data to follow, 
a global alpha value (0-255), and a dirty level (see above). The size is a 
32-bit native-endian unsigned integer; the other two fields are each one-byte
unsigned integers. The header field can be generated and sent in Ruby by 
something like: 

```
STDOUT.write([data_size, alpha, dirty_level].pack("LCC"))
```

The size can be zero; this indicates "no change" to the keyer. This can be
used to keep CPU usage down; many channel types will re-use previous frames
if they receive a zero-size frame. If you send a size of zero, don't send
any data.

The data sent will vary depending on the type of channel being used. For PNG
it should be the entire contents of a PNG file. For SVG it should be a 
complete SVG document. Off-by-one errors with the size sent can cause strange
hangups as the keyer will be getting less data than it expects, or corrupted
headers, so be careful. Other pitfalls include error-handling code that logs
messages to stdout instead of stderr. These will corrupt the data transfer
and cause hangups.

## JavaScript-Driven Keying
The `JsCharacterGenerator` channel can be used to create composite graphics
driven by a piece of JavaScript. The graphics or animations are composed 
from a set of images loaded from disk. 

Here's a simple example script to load and render a static image:

```
({
	image: load_asset('/path/to/image.tga'),
	render: function() {
		draw(this.image, 0, 0, 0, 0, -1, -1);
	}
});
```

See examples [here](https://gist.github.com/asquared/9500261) and
[here](https://github.com/rpitv/rpits/blob/master/test_head.js).

### load_asset
```
function load_asset(pathname) { ... }
```
Use to load TGA-format images into the javascript. Return value is an integer
handle representing the loaded image, or -1 if the image failed to load.

### draw
```
function draw(
	asset_handle, src_x, src_y,
	dest_x, dest_y, w, h,
	alpha
) { ... }
```
Call within your render method to render a portion of an asset. You start out
with an empty (fully-transparent) framebuffer and each draw() call layers on
top of the previous calls. Set `w` to -1 to use the full width of the asset 
and `h` to -1 to use the full height. `alpha` range is 0 to 255.

### Keyer Configuration
```
js_channel = JsCharacterGenerator.new('./js_keyer_server.rb')
js_channel.set_x 0
js_channel.set_y 0
keyer.cg js_channel
```

### HTTP API
`js_keyer_server.rb` exports a simple HTTP API for the JavaScript keyer 
channel. It listens on localhost port 3004. PUT some JavaScript to `/script`
to send in a new script. PUT some JSON to `/command` and it will be passed
to the currently-loaded script's `command` method, if any exists.

## HTML5 Live Keying (still very experimental)
HTML live keying is supported by the `ShmCharacterGenerator` (shared memory).
You will need to download and install 
[exacore-cef](https://github.com/exavideo/exacore-cef-plugin) in order to use
this facility. An example configuration can be found in the sample 
configuration file `keyer_run.rb`.

