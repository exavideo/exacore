require_relative '../input/input'

evt = Input::InputEvent.new
fd = File.open("/dev/shuttlepro")

while true
    ret = Input::read_event(fd.fileno, evt)
    puts "#{ret} #{evt.type} #{evt.code} #{evt.value}"
end
