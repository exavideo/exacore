function loadJson(aUrl, callback) {
    jQuery.ajax(aUrl, {
        success: function(data, jqXHR, textStatus) {
            callback(data);
        },
        dataType: "json"
    });
}

function loadHtml(aUrl, callback) {
    jQuery.ajax(aUrl, {
        success: function(data, jqXHR, textStatus) {
            callback(data);
        },
        dataType: "html"
    });
}

function postJson(aUrl, data) {
    jQuery.ajax(aUrl, {
        type: 'POST',
        data: JSON.stringify(data),
        dataType: "json"
    });
}

function putJson(aUrl, data) {
    jQuery.ajax(aUrl, {
        type: 'PUT',
        data: JSON.stringify(data),
        dataType: "json"
    });
}

function deleteJson(data, aUrl, callback) {
    jQuery.ajax(aUrl, {
        type: 'DELETE',
        success: function(data, jqXHR, textStatus) {
            callback(data);
        },
        data: JSON.stringify(data),
        dataType: "json"
    });
}

function get_timeline_frames(pos) {
    var ret = []
    for (var i = -7; i < 3; i++) {
        ret.push(pos + i * 30);
    }
    return ret;
}

function update_timelines(evt) {
    $.each(evt.shots, function(i, shot) {
        update_timeline(shot);
    });
}

function update_timeline(shot) {
    var timeline_div = $('div#source' + shot.source);
    var frames = get_timeline_frames(shot.start);

    timeline_div.data('shot', shot);

    $.each(frames, function(i, f) {
        var im = $(timeline_div.find('img')[i]);
        im.attr('src', '/sources/' + shot.source + '/' + f + '/thumbnail.jpg');
        im.data('frame', f);
    });
}

function make_timeline(shot) {
    var timeline_div = $('<div class="timeline"></div>');
    timeline_div.attr('id', 'source' + shot.source);
    timeline_div.data('shot', shot);
    
    var frames = get_timeline_frames(shot.start);

    $.each(frames, function(i, f) {
        var im = $('<img />');
        im.attr('src', '/sources/' + shot.source + '/' + f + '/thumbnail.jpg');
        im.data('frame', f);
        timeline_div.append(im);
    });

    timeline_div.mousedown(function(mouseevt) {
        if (mouseevt.shiftKey) {
            var x = mouseevt.pageX - this.offsetLeft;
            // each frame is 120px wide...
            var leftim = timeline_div.find('img')[0];
            // FIXME: shouldn't hardcode the 9
            var rightim = timeline_div.find('img')[9];

            var lpos = leftim.offsetLeft + leftim.width / 2;
            var rpos = rightim.offsetLeft + rightim.width / 2;

            var lframe = $(leftim).data('frame');
            var rframe = $(rightim).data('frame');

            var markframe = Math.floor((x - lpos) / (rpos - lpos) * (rframe - lframe));
            
            console.log("marked frame " + markframe);

            if (mouseevt.which == 1) {
                timeline_div.data('mark_in', markframe);
            } else if (mouseevt.which == 3) {
                timeline_div.data('mark_out', markframe);
            }
        } else if (mouseevt.ctrlKey) {
            /* make shot and add to queue */
            var source = timeline_div.data('shot').source;
            var mark_in = timeline_div.data('mark_in');
            var mark_out = timeline_div.data('mark_out');
            if (mark_out < mark_in) {
                var tmp = mark_out;
                mark_out = mark_in;
                mark_in = tmp;
            }
            queue_shot(source, mark_in, mark_out);
        }
    });

    /* disable context menu */
    timeline_div.find('img').bind('contextmenu', function(e) {
        e.preventDefault( );
        return false;
    });


    $('#timelines').append(timeline_div)
}

function next_event( ) {
    var event_id = $('#timelines').data('event') + 1;
    var events = $('#timelines').data('events');

    if (event_id < events.length) {
        update_timelines(events[event_id]);
        $('#timelines').data('event', event_id);
    }
}

function prev_event( ) {
    var event_id = $('#timelines').data('event') - 1;
    var events = $('#timelines').data('events');

    if (event_id >= 0) {
        update_timelines(events[event_id]);
        $('#timelines').data('event', event_id);
    }
}

function filter_events(events) {
    return events;
}

function zero_pad(tcval) {
    if (tcval < 10) {
        return '0' + tcval;
    } else {
        return tcval;
    }
}

function format_tc(tc) {
    var frames = tc % 30;
    var seconds = Math.floor(tc / 30);

    minutes = Math.floor(seconds / 60);
    seconds = seconds % 60;

    hours = Math.floor(minutes / 60);
    minutes = minutes % 60;

    return zero_pad(hours) + ':' + zero_pad(minutes) + ':' +
            zero_pad(seconds) + ':' + zero_pad(frames);
}

function queue_shot(source, start, end) {
    var shot = {
        source : source,
        start : start,
        length : end - start + 1,
    }

    var shot_div = $('<div></div>');
    shot_div.text('source ' + source + ' ' + format_tc(start)
            + '-' + format_tc(end));

    shot_div.data('shot', shot);
    $('#reel').append(shot_div);
}

function clear_reel( ) {
    $('#reel').empty( );
}

function rollout_reel( ) {
    var shots = $('#reel div').map(function(i, e) {
        return $(e).data('shot');
    }).get( );

    putJson('/roll_queue.json', shots);
}

function updateEvents(events) {
    if ($('#timelines .timeline').length == 0 && events.length > 0) {
        // create timelines
        $.each(events[0].shots, function(i, shot) {
            make_timeline(shot);
        })

        $('#timelines').data('event', 0);
    }

    $('#timelines').data('all_events', events);
    $('#timelines').data('events', filter_events(events));
}

function updateEventsTimeout( ) {
    console.log("requested event data");
    loadJson('/events.json', updateEvents);
    setTimeout(updateEventsTimeout, 1000);
}

function loadFiles( ) {
    loadJson('/files.json', function(files) {
        $.each(files, function(i, x) {
            $('#filepicker').append('<option value="'+x+'">'+x+'</option>');
        });
    });
}

function rolloutFile( ) {
    var file = $('#filepicker').val( );

    if (file.length > 0) {
        putJson('/ffmpeg_rollout.json', { filename : file });
    }
}

function resumeEncode( ) {
    putJson('/resume_encode.json', { });
}

$(function() {
    updateEventsTimeout( );
    loadFiles( );

    $('#clear_queue').click(function() { clear_reel( ); });
    $('#roll_queue').click(function() { rollout_reel( ); });
    $('#prev_event').click(prev_event);
    $('#next_event').click(next_event);

    $('#rollout_file').click(rolloutFile);
    $('#resume_encode').click(resumeEncode);
});

