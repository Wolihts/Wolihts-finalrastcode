#define SOKOL_IMPL
#define SOKOL_GLCORE
#include "./includes/sokol_app.h"
#include "./includes/sokol_gfx.h"
#include "./includes/sokol_log.h"
#include "./includes/sokol_glue.h"
#include "./includes/file_util.h"
#include <cmath>
#include <iostream>
#include <vector>
#include <string>

using namespace std;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct InputState {
    float mouse_x = 0;
    float mouse_y = 0;

    float prev_mouse_x = 0;
    float prev_mouse_y = 0;
    float mouse_dx = 0;
    float mouse_dy = 0;
    float xRotation = 0;
    float yRotation = 0;
    float camX = 0;
    float camY = 0;
    float camZ = 5;

    bool keys[512];
};
InputState input;

struct vs_params_t {
    float model[16];
    float view_proj[16];
};

struct fs_params_t {
    float time;
};
// shader - used similar idea to ANIMATED PERLIN NOISE - UNLIT, LAVA / FIRE EFFECT - UNLIT, ANIMATED VORONOI CELLS - UNLIT
struct State {
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;
    int index_count;
    float time;
};
struct State state;

struct v3 {
    float x;
    float y;
    float z;
};

struct v2 {
    float x;
    float y;
};

struct Id {
    int p;
    int u;
    int n;
};

struct fce {
    Id a;
    Id b;
    Id c;
};

struct vrt {
    v3 p;
    v3 n;
    v2 u;
};

vector<string> split_line(string line, char c)
{
    vector<string> out;
    string cur = "";

    for (int i = 0; i < (int)line.size(); i++) {
        if (line[i] == c) {
            if (cur != "") {
                out.push_back(cur);
                cur = "";
            }
        }
        else {
            cur += line[i];
        }
    }

    if (cur != "") {
        out.push_back(cur);
    }

    return out;
}
void event(const sapp_event* e) {
    switch(e->type) {
        case SAPP_EVENTTYPE_KEY_DOWN:
            input.keys[e->key_code] = true;
            break;
        case SAPP_EVENTTYPE_KEY_UP:
            input.keys[e->key_code] = false;
            break;
        default:
            break;
    }
}

void init(void) {
    sg_desc desc = {};
    desc.environment = sglue_environment();
    desc.logger.func = slog_func;
    sg_setup(&desc);

    input.camZ = 5.0f;
    state.time = 0.0f;
    vector<v3> ps;
    vector<v2> us;
    vector<v3> ns;
    vector<fce> fs;
    char* obj = load_file("Untitled2.obj");
    vector<string> ls = split(obj);
    free(obj);

    for (int i = 0; i < (int)ls.size(); i++) {
        string line = ls[i];

        if (line.size() < 2) {
            continue;
        }
        vector<string> parts = split_line(line, ' ');
        if (parts.size() == 0) {
            continue;
        }
        string t = parts[0];

        if (t == "v") {
            if (parts.size() < 4) {
                continue;
            }
            v3 p;
            p.x = stof(parts[1]);
            p.y = stof(parts[2]);
            p.z = stof(parts[3]);
            ps.push_back(p);
        }
        else if (t == "vt") {
            if (parts.size() < 3) {
                continue;
            }
            v2 u;
            u.x = stof(parts[1]);
            u.y = stof(parts[2]);
            us.push_back(u);
        }
        else if (t == "vn") {
            if (parts.size() < 4) {
                continue;
            }
            v3 n;
            n.x = stof(parts[1]);
            n.y = stof(parts[2]);
            n.z = stof(parts[3]);
            ns.push_back(n);
        }
        else if (t == "f") {
            vector<Id> ids;
            for (int j = 1; j < (int)parts.size(); j++) {
                vector<string> nums = split_line(parts[j], '/');

                Id id;
                id.p = -1;
                id.u = -1;
                id.n = -1;

                if (nums.size() > 0 && nums[0] != "") {
                    id.p = stoi(nums[0]) - 1;
                }

                if (nums.size() > 1 && nums[1] != "") {
                    id.u = stoi(nums[1]) - 1;
                }

                if (nums.size() > 2 && nums[2] != "") {
                    id.n = stoi(nums[2]) - 1;
                }

                ids.push_back(id);
            }
            for (int j = 1; j + 1 < (int)ids.size(); j++) {
                fce f;
                f.a = ids[0];
                f.b = ids[j];
                f.c = ids[j + 1];
                fs.push_back(f);
            }
        }
    }
    vector<float> vertices;
    vector<uint16_t> indices;

    for (int i = 0; i < (int)fs.size(); i++) {
        Id ids[3];
        ids[0] = fs[i].a;
        ids[1] = fs[i].b;
        ids[2] = fs[i].c;
        for (int j = 0; j < 3; j++) {
            vrt v;
            v.p = ps[ids[j].p];

            if (ids[j].n >= 0 && ids[j].n < (int)ns.size()) {
                v.n = ns[ids[j].n];
            }
            else {
                v.n.x = 0.0f;
                v.n.y = 0.0f;
                v.n.z = 1.0f;
            }
            if (ids[j].u >= 0 && ids[j].u < (int)us.size()) {
                v.u = us[ids[j].u];
            }
            else {
                v.u.x = 0.0f;
                v.u.y = 0.0f;
            }
            vertices.push_back(v.p.x);
            vertices.push_back(v.p.y);
            vertices.push_back(v.p.z);

            vertices.push_back(v.n.x);
            vertices.push_back(v.n.y);
            vertices.push_back(v.n.z);

            vertices.push_back(v.u.x);
            vertices.push_back(v.u.y);

            indices.push_back((uint16_t)indices.size());
        }
    }
    state.index_count = (int)indices.size();
    sg_buffer_desc vbuf_desc = {};
    vbuf_desc.data = { vertices.data(), vertices.size() * sizeof(float) };
    state.bind.vertex_buffers[0] = sg_make_buffer(&vbuf_desc);

    sg_buffer_desc ibuf_desc = {};
    ibuf_desc.usage.index_buffer = true;
    ibuf_desc.usage.vertex_buffer = false;
    ibuf_desc.data = { indices.data(), indices.size() * sizeof(uint16_t) };
    state.bind.index_buffer = sg_make_buffer(&ibuf_desc);

    char* vs_src = load_file("vertex.glsl");
    char* fs_src = load_file("fragment.glsl");

    sg_shader_desc shd_desc = {};
    shd_desc.vertex_func.source = vs_src;
    shd_desc.fragment_func.source = fs_src;

    shd_desc.attrs[0].glsl_name = "position";
    shd_desc.attrs[1].glsl_name = "normal";
    shd_desc.attrs[2].glsl_name = "uv";
    shd_desc.uniform_blocks[0].stage = SG_SHADERSTAGE_VERTEX;
    shd_desc.uniform_blocks[0].size = sizeof(vs_params_t);
    shd_desc.uniform_blocks[0].layout = SG_UNIFORMLAYOUT_NATIVE;
    shd_desc.uniform_blocks[0].glsl_uniforms[0] = { SG_UNIFORMTYPE_MAT4, 0, "model" };
    shd_desc.uniform_blocks[0].glsl_uniforms[1] = { SG_UNIFORMTYPE_MAT4, 0, "view_proj" };
    shd_desc.uniform_blocks[1].stage = SG_SHADERSTAGE_FRAGMENT;
    shd_desc.uniform_blocks[1].size = sizeof(fs_params_t);
    shd_desc.uniform_blocks[1].layout = SG_UNIFORMLAYOUT_NATIVE;
    shd_desc.uniform_blocks[1].glsl_uniforms[0] = { SG_UNIFORMTYPE_FLOAT, 0, "time" };

    sg_shader shd = sg_make_shader(&shd_desc);
    free(vs_src);
    free(fs_src);

    sg_pipeline_desc pip_desc = {};
    pip_desc.shader = shd;
    pip_desc.index_type = SG_INDEXTYPE_UINT16;
    pip_desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;
    pip_desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT3;
    pip_desc.layout.attrs[2].format = SG_VERTEXFORMAT_FLOAT2;
    pip_desc.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;
    pip_desc.depth.write_enabled = true;
    pip_desc.cull_mode = SG_CULLMODE_BACK;
    pip_desc.face_winding = SG_FACEWINDING_CCW;
    state.pip = sg_make_pipeline(&pip_desc);

    state.pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
    state.pass_action.colors[0].clear_value = { 0.05f, 0.06f, 0.10f, 1.0f };
    state.pass_action.depth.load_action = SG_LOADACTION_CLEAR;
    state.pass_action.depth.clear_value = 1.0f;
}

void frame(void) {
    sg_pass pass = {};
    pass.action = state.pass_action;
    pass.swapchain = sglue_swapchain();
    sg_begin_pass(&pass);
    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);

    float turn_speed = 0.025f;

    if(input.keys[SAPP_KEYCODE_Q]) {
        input.yRotation -= turn_speed;
    }
    if(input.keys[SAPP_KEYCODE_E]) {
        input.yRotation += turn_speed;
    }
    if(input.keys[SAPP_KEYCODE_R]) {
        input.xRotation -= turn_speed;
    }
    if(input.keys[SAPP_KEYCODE_F]) {
        input.xRotation += turn_speed;
    }
    float speed = 0.05f;
    float s = sinf(input.yRotation);
    float c = cosf(input.yRotation);

    if(input.keys[SAPP_KEYCODE_W]) {
        input.camX += s * speed;
        input.camZ -= c * speed;
    }
    if(input.keys[SAPP_KEYCODE_S]) {
        input.camX -= s * speed;
        input.camZ += c * speed;
    }
    if(input.keys[SAPP_KEYCODE_D]) {
        input.camX += c * speed;
        input.camZ += s * speed;
    }
    if(input.keys[SAPP_KEYCODE_A]) {
        input.camX -= c * speed;
        input.camZ -= s * speed;
    }
    if(input.keys[SAPP_KEYCODE_SPACE]) {
        input.camY -= speed;
    }
    if(input.keys[SAPP_KEYCODE_LEFT_SHIFT] || input.keys[SAPP_KEYCODE_RIGHT_SHIFT]) {
        input.camY += speed;
    }
    vs_params_t vs_params;

    float xRotateCosine = cosf(input.xRotation);
    float xRotateSin = sinf(input.xRotation);
    float yRotateCosine = cosf(input.yRotation - 1.57079632f);
    float yRotateSin = sinf(input.yRotation - 1.57079632f);

    float* m = vs_params.model;
    m[0] = -yRotateCosine;  m[4] = yRotateSin * xRotateSin;  m[8]  = yRotateSin * xRotateCosine; m[12] = -input.camX;
    m[1] = 0.0f; m[5] = xRotateCosine; m[9] = -xRotateSin; m[13] = -input.camY;
    m[2] = yRotateSin; m[6] = yRotateCosine * xRotateSin; m[10] = yRotateCosine * xRotateCosine; m[14] = -input.camZ;
    m[3] = 0.0f; m[7] = 0.0f; m[11] = 0.0f; m[15] = 1.0f;

    float* p = vs_params.view_proj;
    p[0] = 0.5f; p[4] = 0.0f; p[8] = 0.0f; p[12] = 0.0f;
    p[1] = 0.0f; p[5] = 0.5f; p[9] = 0.0f; p[13] = 0.0f;
    p[2] = 0.0f; p[6] = 0.0f; p[10] = -0.05f; p[14] = 0.0f;
    p[3] = 0.0f; p[7] = 0.0f; p[11] = 0.0f; p[15] = 1.0f;
    sg_range params_range = {};
    params_range.ptr = &vs_params;
    params_range.size = sizeof(vs_params);
    sg_apply_uniforms(0, &params_range);

    state.time += 0.016f;

    fs_params_t fs_params;
    fs_params.time = state.time;
    sg_range fs_range = {};
    fs_range.ptr = &fs_params;
    fs_range.size = sizeof(fs_params);
    sg_apply_uniforms(1, &fs_range);
    sg_draw(0, state.index_count, 1);
    sg_end_pass();
    sg_commit();
}

void cleanup(void) {
    sg_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {
    sapp_desc app = {};
    app.init_cb = init;
    app.frame_cb = frame;
    app.cleanup_cb = cleanup;
    app.event_cb = event;
    app.width = 800;
    app.height = 800;
    app.window_title = "Rasterizer";
    app.logger.func = slog_func;

    return app;
}