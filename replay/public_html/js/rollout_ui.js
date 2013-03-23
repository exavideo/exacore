/*
 * Copyright 2011, 2012, 2013 Exavideo LLC.
 * 
 * This file is part of openreplay.
 * 
 * openreplay is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * openreplay is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with openreplay.  If not, see <http://www.gnu.org/licenses/>.
 */

"use strict";

function putJson(data, aUrl, callback) {
    jQuery.ajax(aUrl, {
        type: 'PUT',
        success: function(data, jqXHR, textStatus) {
            callback(data);
        },
        data: JSON.stringify(data),
        dataType: "json"
    });
}

$(function() {
    load_available_list( );
    $(".sortableList").sortable({
        connectWith: ".sortableList"
    }).disableSelection( );

    $(".sortableList").bind("sortstop", function() { update_rollout_duration(); });
    $("#rolloutButton").click(function() { begin_rollout( ); });

    setInterval(function() { update_rollout_state( ) }, 20);
});

function update_rollout_state( ) {
    $.getJSON('/rollstate.json', function(data) {
        var duration = data['duration']
        var position = data['position']
        $("#timeRolled .timer").text(format_timecode_no_frames(position));
        if (duration < 0) {
            $("#timeRemaining .timer").text("unknown");
        } else {
            var remaining = duration - position;
            $("#timeRemaining .timer").text(format_timecode_no_frames(remaining));
        }
    });
}

function update_rollout_duration( ) {
    var sum = 0;

    $("#rolloutList li").each(function(i,elem) {
        console.log(elem);
        var filedata = $(elem).data();
        console.log(filedata);
        sum += filedata['duration'];
    });

    $("#totalDuration").text(format_timecode(sum));
}

function load_available_list( ) {
    $.getJSON('/fileinfo.json', function(data) {
        $.each(data, function(i, file) {
            add_file_to_available_list(file);
        });
    });
}

function add_file_to_available_list(file) {
    var elem = $('<li><span class="path"></span><br /><span class="duration"></span></li>');
    elem.data(file);
    elem.find(".path").text(file['basename']);
    elem.find(".duration").text(format_timecode(file['duration']));
    $("#availableList").append(elem);
}

function begin_rollout( ) {
    var list = $("#rolloutList li").map(function(i,e) {
        return $(e).data( )['path'];
    }).get( );
    putJson(list, '/roll_queue.json', function() { });

}

function format_timecode(tc) {
    var frames = tc % 30;
    var seconds = Math.floor(tc / 30);
    var minutes = Math.floor(seconds / 60);
    seconds = seconds % 60;
    var hours = Math.floor(minutes / 60);
    minutes = minutes % 60;

    return [
        leading_zero(hours), 
        leading_zero(minutes), 
        leading_zero(seconds),
        leading_zero(frames)
    ].join(':');
}

function format_timecode_no_frames(tc) {
    var seconds = Math.floor(tc / 30);
    var minutes = Math.floor(seconds / 60);
    seconds = seconds % 60;
    var hours = Math.floor(minutes / 60);
    minutes = minutes % 60;

    return [
        leading_zero(hours), 
        leading_zero(minutes), 
        leading_zero(seconds),
    ].join(':');
}

function leading_zero(number) {
    if (number < 10) {
        return '0' + number.toString( );
    } else {
        return number.toString( );
    }
}
