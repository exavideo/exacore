// upload stuff to the keying server...

function putPng(desturl, obj) {
    jQuery.ajax({
        type: "PUT",
        url: desturl,
        contentType: "image/png",
        data: obj,
        error: function(jqxhr, textStatus) {
            //alert("Communication error: " + textStatus);  
        }
    });
}

function postJson(desturl, obj) {
    jQuery.ajax({
        type: "POST",
        url: desturl,
        contentType: "application/json",
        data: JSON.stringify(obj),
        error: function(jqxhr, textStatus) {
            //alert("Communication error: " + textStatus);  
        }
    });
}

$(document).ready(function() {
    $("#pngImage").change(function() {
        var file = $("#pngImage")[0].files[0];
        var fr = new FileReader();
        fr.onload = function(e) {
            var data = e.target.result;
            putPng('/key_dataurl', data);
        };
        fr.readAsDataURL(file);
    });


    $("#titleUp").click(function() {
        postJson('/dissolve_in/30', {});
    });

    $("#titleDown").click(function() {
        postJson('/dissolve_out/30', {});
    });
});
