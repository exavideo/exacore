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

#include "js_character_generator_script.h"
#include <string.h>

using namespace v8;

static void load_asset_callback(const v8::FunctionCallbackInfo<Value> &args) {
	JsCharacterGeneratorScript *script;
	Local<External> data = Local<External>::Cast(args.Data( ));
	script = (JsCharacterGeneratorScript *) data->Value( );

	if (args.Length() == 1) {
		String::Utf8Value path_ascii(args[0]);
		args.GetReturnValue().Set(script->load_asset(*path_ascii));
	}
}

static void draw_callback(const v8::FunctionCallbackInfo<Value> &args) {
	JsCharacterGeneratorScript *script;
	Local<External> data = Local<External>::Cast(args.Data( ));
	script = (JsCharacterGeneratorScript *) data->Value( );

	if (args.Length() == 8) {
		/* call draw function */
		script->draw(
			args[0]->IntegerValue(),
			args[1]->IntegerValue(),
			args[2]->IntegerValue(),
			args[3]->IntegerValue(),
			args[4]->IntegerValue(),
			args[5]->IntegerValue(),
			args[6]->IntegerValue(),
			args[7]->IntegerValue()
		);		
	}
}

static void check_isolate( ) {
	Isolate *isolate = Isolate::GetCurrent( );
	if (isolate == NULL) {
		isolate = Isolate::New( );
		isolate->Enter( );
	}
}

JsCharacterGeneratorScript::JsCharacterGeneratorScript(
	const char *script, 
	size_t size
) {
	check_isolate( );
	Isolate *isolate = Isolate::GetCurrent( );
	HandleScope handle_scope(isolate);

	(void) size; /* FIXME */

	/* set up global functions in v8 */
	Handle<ObjectTemplate> global_template = ObjectTemplate::New( );

	global_template->Set(
		String::NewFromUtf8(isolate, "load_asset"), 
		FunctionTemplate::New(
			isolate,
			load_asset_callback, 
			External::New(isolate, this)
		)
	);

	global_template->Set(
		String::NewFromUtf8(isolate, "draw"),
		FunctionTemplate::New(
			isolate,
			draw_callback,
			External::New(isolate, this)
		)
	);

	Handle<Context> context = Context::New(isolate, NULL, global_template);
	v8_context.Reset(isolate, context);
	Context::Scope ctxscope(context);

	Handle<Script> v8_script = Script::Compile(String::NewFromUtf8(isolate, script));
	if (v8_script.IsEmpty( )) {
		throw std::runtime_error("JS compilation failed");
	}

	Handle<Value> script_result = v8_script->Run( );
	if (script_result.IsEmpty( )) {
		throw std::runtime_error("JS execution failed");
	}

	Handle<Object> object = Handle<Object>::Cast(script_result);
	v8_object.Reset(isolate, object);

}

JsCharacterGeneratorScript::~JsCharacterGeneratorScript( ) {
	/* free all our assets */
	for (RawFrame *&frame : assets) {
		delete frame;
	}

	/* do we need this? */
	v8_object.Reset( );
	v8_context.Reset( );
}

RawFrame *JsCharacterGeneratorScript::render_frame( ) {
	Isolate *isolate = Isolate::GetCurrent( );
	HandleScope handle_scope(isolate);
	TryCatch try_catch;

	RawFrame *ret;
	current_frame = new RawFrame(1920, 1080, RawFrame::BGRAn8);
	memset(current_frame->data( ), 0, current_frame->size( ));

	Local<Object> title_obj = Local<Object>::New(isolate, v8_object);

	Handle<Function> render_func = Handle<Function>::Cast(
		title_obj->Get(String::NewFromUtf8(isolate, "render"))
	);

	render_func->Call(title_obj, 0, NULL);

	if (try_catch.HasCaught( )) {
		fprintf(stderr, "caught javascript exception - what now?\n");
	}

	ret = current_frame;
	current_frame = NULL;
	return ret;	
}


unsigned int JsCharacterGeneratorScript::load_asset(const char *path) {
	RawFrame *asset = RawFrame::from_image_file(path);
	fprintf(stderr, "JsCharacterGeneratorScript: load asset %s\n", path);

	assets.push_back(asset);
	return assets.size( ) - 1;	
}

void JsCharacterGeneratorScript::draw(
	unsigned int asset_id,
	coord_t src_x, coord_t src_y,
	coord_t dest_x, coord_t dest_y,
	coord_t w, coord_t h, uint8_t galpha
) {
	current_frame->draw->alpha_composite(
		dest_x, dest_y, 
		assets[asset_id], 
		src_x, src_y, 
		w, h, 
		galpha
	);
}


void JsCharacterGeneratorScript::send_message(const char *data, size_t size) {
	Isolate *isolate = Isolate::GetCurrent( );
	HandleScope handle_scope(isolate);
	Local<Object> title_obj = Local<Object>::New(isolate, v8_object);
	Handle<String> name = String::NewFromUtf8(isolate, "command");

	(void) size;

	if (title_obj->Has(name)) {
		Handle<Function> command_func = Handle<Function>::Cast(
			title_obj->Get(name)
		);

		Handle<Value> message = String::NewFromUtf8(isolate, data);
		command_func->Call(title_obj, 1, &message);
	}
}

unsigned int JsCharacterGeneratorScript::dirty_level( ) {
#if 0
	Isolate *isolate = Isolate::GetCurrent( );
	HandleScope handle_scope(isolate);
	Local<Object> title_obj = Local<Object>::New(isolate, v8_object);
	Handle<String> name = String::NewFromUtf8(isolate, "dirty_level");
	
	if (title_obj->Has(name)) {
		Handle<Value> dirty_level = title_obj->Get(name);

		if (dirty_level.IsEmpty( )) {
			return 0;
		} else {
			return dirty_level->IntegerValue( );
		}
	} else {
		return 0;	/* default dirty level if none defined */
	}
#else
	return 0;
#endif
}
