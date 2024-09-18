#include <windows.h>
#include <vector>
#include <cmath>
#include <fstream>
#include "C_API.h"

//kad se projekat kreira -> create cpp project using existing resources
//za pokretanje -> Release mod, a ne Debug u  konfiguraciji i obrisati ako ima L"TEXT" -> "TEXT"
// i wchar_t promeniti u char

std::ofstream camera_position_file("camera_positions.txt");

renderer rend;

bool updated = true;

enum class Mode {
    EXPLORE, PICTURE
};

Mode mode = Mode::EXPLORE;

const int WIDTH = 800;
const int HEIGHT = 600;

BITMAPINFO bmi;
HBITMAP hBitmap;
HDC hMemDC;
HBITMAP oldBitmap;
COLORREF *pBitmapData;

LARGE_INTEGER frequency;
LARGE_INTEGER renderStart, renderEnd, displayStart, displayEnd, frameStart, frameEnd;
double renderTime = 0.0;
double displayTime = 0.0;

POINT windowCenter;

void initBitmap(HWND hwnd) {
    hMemDC = CreateCompatibleDC(GetDC(hwnd));

    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = WIDTH;
    bmi.bmiHeader.biHeight = -HEIGHT; // Top-down DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = 0;
    bmi.bmiHeader.biXPelsPerMeter = 0;
    bmi.bmiHeader.biYPelsPerMeter = 0;
    bmi.bmiHeader.biClrUsed = 0;
    bmi.bmiHeader.biClrImportant = 0;

    hBitmap = CreateDIBSection(hMemDC, &bmi, DIB_RGB_COLORS, (void **)&pBitmapData, NULL, 0);
    oldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);
}

void render() {
    QueryPerformanceCounter(&renderStart);

    renderer_render(&rend, get_scene());

    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            vec3 color = renderer_get_color(&rend, HEIGHT - 1 - y, x);
            pBitmapData[y * WIDTH + x] = RGB(float_real(color.x) * 255, float_real(color.y) * 255, float_real(color.z) * 255);
        }
    }

    QueryPerformanceCounter(&renderEnd);
    renderTime = double(renderEnd.QuadPart - renderStart.QuadPart) / frequency.QuadPart;
}

void snapPicture(HWND hwnd) {
    camera *cam = scene_get_camera(get_scene());
    mode = Mode::PICTURE;
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            real u = real_int(x - 400) >> 9;
            real v = real_int(HEIGHT - 1 - y - 300) >> 9;
            vec3 temp0 = camera_get_position(cam), temp1 = camera_get_ray_direction(cam, u, v);
            ray ray = ray_init(&temp0, &temp1);
            vec3 color = scene_trace_ray(get_scene(), &ray, 0);
            pBitmapData[y * WIDTH + x] = RGB(float_real(color.x) * 255, float_real(color.y) * 255, float_real(color.z) * 255);
        }
    }
    InvalidateRect(hwnd, NULL, FALSE);
    UpdateWindow(hwnd);
}

void display(HWND hwnd) {
    QueryPerformanceCounter(&displayStart);

    HDC hdc = GetDC(hwnd);
    BitBlt(hdc, 0, 0, WIDTH, HEIGHT, hMemDC, 0, 0, SRCCOPY);
    ReleaseDC(hwnd, hdc);

    QueryPerformanceCounter(&displayEnd);
    displayTime = double(displayEnd.QuadPart - displayStart.QuadPart) / frequency.QuadPart;
}

real SPEED_YAW = real_float(0.6f);
real SPEED_PITCH = real_float(0.6f);
real SPEED_MOVE = real_mul(REAL_PI, SPEED_YAW);

real speed_move, speed_yaw, speed_pitch;
int tab_pressed = 0;
int esc_pressed = 0;

bool is_sphere_creating = false;
real sphere_radius = real_float(1.0f);
vec3 sphere_position;

POINT last_mouse_pos;
bool right_mouse_pressed = false;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    scene *sc = get_scene();
    switch (uMsg) {
    case WM_MOUSEMOVE:
    {
        if (mode == Mode::EXPLORE) {
            // Handle mouse movement to adjust camera
            POINT currentMousePos;
            GetCursorPos(&currentMousePos);

            real deltaX = (currentMousePos.x - windowCenter.x) << 7;
            real deltaY = (currentMousePos.y - windowCenter.y) << 7;

            camera *cam = scene_get_camera(get_scene());

            real yaw_change = -real_mul(deltaX, SPEED_YAW);
            real pitch_change = -real_mul(deltaY, SPEED_PITCH);

            real yaw = camera_get_yaw(cam);
            real pitch = camera_get_pitch(cam);

            yaw = real_add(yaw, yaw_change);
            pitch = real_add(pitch, pitch_change);

            if (pitch > REAL_PI_TWO) pitch = REAL_PI_TWO;
            if (pitch < -REAL_PI_TWO) pitch = REAL_PI_TWO;

            camera_set_yaw(cam, yaw);
            camera_set_pitch(cam, pitch);
            camera_update_angles(cam);

            last_mouse_pos = currentMousePos;

            SetCursorPos(windowCenter.x, windowCenter.y);

            updated = true;
        }
        break;
    }
    case WM_RBUTTONUP:
    {
        if (mode == Mode::EXPLORE) {
            if (!sc->ghost_mode) {
                right_mouse_pressed = true;
                sphere_position = camera_get_forward(&sc->cam);
                sc->ghost_mode = true;
                sc->ghost_radius = 0x00010000;
                updated = true;
            }
            else {
                sc->ghost_mode = false;
                vec3 c = camera_get_position(&sc->cam), forward = camera_get_forward(&sc->cam);
                forward = vec3_multiply_scalar(&forward, SCENE_SPAWN_DISTANCE + sc->ghost_radius);
                c = vec3_add(&c, &forward);
                material m = { material_color_rgb(0xFFFF00), real_float(0.5f) };
                sphere new_sphere = sphere_init_with_values(c, sc->ghost_radius, m);
                scene_add_sphere(sc, &c, sc->ghost_radius);
            }
        }
        break;
    }
    case WM_LBUTTONUP:
    {
        // shoot ray and the first sphere it hits (if any) is marked

        real min_t = 0x7fffffff;
        sphere *min_s = nullptr;
        for (int i = 0; i < sc->num_of_spheres; i++) {
            real t;
            vec3 forward = camera_get_forward(&sc->cam);
            ray r = ray_init(&sc->cam.position, &forward);
            if (sphere_intersect(&sc->spheres[i], &r, &t)) {
                if (t < min_t) {
                    min_t = t;
                    min_s = &sc->spheres[i];
                }
            }
        }
        sc->marked = min_s;
        updated = true;
        break;
    }
    case WM_MOUSEWHEEL:
    {
        if (sc->ghost_mode) {
            int mouse_delta = GET_WHEEL_DELTA_WPARAM(wParam);
            const real radius_delta = real_float(0.03f);
            const real radius_min = real_float(0.3f);
            if (mouse_delta > 0) {
                sc->ghost_radius = real_add(sc->ghost_radius, radius_delta);
            }
            else {
                sc->ghost_radius = real_sub(sc->ghost_radius, radius_delta);
                if (sc->ghost_radius < radius_min) {
                    sc->ghost_radius = radius_min; // Minimum radius
                }
            }
            updated = true;
        }
        break;
    }
    case WM_PAINT:
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        display(hwnd);
        EndPaint(hwnd, &ps);
        return 0;
    case WM_DESTROY:
        SelectObject(hMemDC, oldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hMemDC);
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

bool escape_initialized = true;

void exploreMode(HWND hwnd, real delta_time) {
    speed_move -= real_mul(delta_time, SPEED_MOVE) >> 1;
    speed_yaw -= real_mul(delta_time, SPEED_YAW) >> 1;
    speed_pitch -= real_mul(delta_time, SPEED_PITCH) >> 1;

    scene *sc = get_scene();
    camera *cam = scene_get_camera(sc);
    vec3 pos = camera_get_position(cam);
    real yaw = camera_get_yaw(cam);
    real pitch = camera_get_pitch(cam);
    vec3 temp_vec3; // helper for vec3 operations
    real temp_real; // helper for real operations

    if (GetAsyncKeyState('W') & 0x8000) {
        temp_vec3 = camera_get_forward(cam);
        temp_real = real_mul(SPEED_MOVE, delta_time);
        temp_vec3 = vec3_multiply_scalar(&temp_vec3, temp_real);
        pos = vec3_add(&pos, &temp_vec3);
        updated = true;
    }
    if (GetAsyncKeyState('S') & 0x8000) {
        temp_vec3 = camera_get_forward(cam);
        temp_real = real_mul(SPEED_MOVE, delta_time);
        temp_vec3 = vec3_multiply_scalar(&temp_vec3, temp_real);
        pos = vec3_subtract(&pos, &temp_vec3);
        updated = true;
    }
    if (GetAsyncKeyState('A') & 0x8000) {
        temp_vec3 = camera_get_right(cam);
        temp_real = real_mul(SPEED_MOVE, delta_time);
        temp_vec3 = vec3_multiply_scalar(&temp_vec3, temp_real);
        pos = vec3_subtract(&pos, &temp_vec3);
        updated = true;
    }
    if (GetAsyncKeyState('D') & 0x8000) {
        temp_vec3 = camera_get_right(cam);
        temp_real = real_mul(SPEED_MOVE, delta_time);
        temp_vec3 = vec3_multiply_scalar(&temp_vec3, temp_real);
        pos = vec3_add(&pos, &temp_vec3);
        updated = true;
    }
    if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
        temp_real = real_mul(SPEED_MOVE, delta_time);
        pos.y = real_add(pos.y, temp_real);
        updated = true;
    }
    if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
        temp_real = real_mul(SPEED_MOVE, delta_time);
        pos.y = real_sub(pos.y, temp_real);
        if (pos.y < real_float(0.1f)) {
            pos.y = real_float(0.1f);
        }
        updated = true;
    }
    if (GetAsyncKeyState(VK_DELETE) & 0x8000) {
        scene_remove_sphere(sc, sc->marked);
        sc->marked = nullptr;
        updated = true;
    }
    if (GetAsyncKeyState(VK_RETURN) & 0x8000) {
        snapPicture(hwnd);  
        return;
    }
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
        if (!esc_pressed) {
            PostQuitMessage(0);
            esc_pressed = 1;
        }
        return;
    }
    else {
        esc_pressed = 0;
    }
    if (GetAsyncKeyState(VK_TAB) & 0x8000) {
        if (!tab_pressed) {
            updated = true;
            material_light_mode = !material_light_mode;
        }
        tab_pressed = 1;
    }
    else {
        tab_pressed = 0;
    }

    camera_set_position(cam, &pos);

    if (is_sphere_creating) {
        scene_add_sphere(get_scene(), &sphere_position, sphere_radius);
        is_sphere_creating = false;
        updated = true;
    }

    if (updated) {
        if (yaw < 0) {
            yaw += REAL_PI_TWO;
        }
        if (yaw >= REAL_PI_TWO) {
            yaw -= REAL_PI_TWO;
        }
        if (pitch < 0) {
            pitch += REAL_PI_TWO;
        }
        if (pitch >= REAL_PI_TWO) {
            pitch -= REAL_PI_TWO;
        }
        camera_set_position(cam, &pos);
        camera_set_yaw(cam, yaw);
        camera_set_pitch(cam, pitch);
        camera_update_angles(cam);

        render();
        InvalidateRect(hwnd, NULL, FALSE);
        UpdateWindow(hwnd);
        updated = false;
    }
}

void pictureMode(HWND hwnd, real delta_time) {
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
        if (!esc_pressed) {
            InvalidateRect(hwnd, NULL, FALSE);
            UpdateWindow(hwnd);
            updated = true;
            mode = Mode::EXPLORE;
            esc_pressed = 1;
        }
        else {
            esc_pressed = 0;
        }
    }
}

void initColors() {
    material_light_mode = 0;

    material_color_sky[0] = material_color_rgb(0x87CEFB);  // Light blue for normal mode
    material_color_sky[1] = material_color_rgb(0x1f009F);  // Neon magenta (purple) for retro mode

    material_color_horizon[0] = material_color_rgb(0xFFFF00);  // Orange-red for normal sunset horizon
    material_color_horizon[1] = material_color_rgb(0xFF00FF);  // Bright yellow for retro mode

    material_color_floor1[0] = material_color_rgb(0xB0E0E6);  // Light foggy blue for normal mode (for a sky-like ground effect)
    material_color_floor1[1] = material_color_rgb(0x00FFFF);  // Bright cyan for retro mode

    material_color_floor2[0] = material_color_rgb(0x2F4F6F);  // Dark gray-blue for normal mode (to mimic sky reflection)
    material_color_floor2[1] = material_color_rgb(0x8A2BE2);  // Neon purple for retro mode

}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    initColors();

    const wchar_t CLASS_NAME[] = L"RayTracingDemo";
    WNDCLASS wc = {};

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Ray Tracing Demo",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, WIDTH, HEIGHT,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    initBitmap(hwnd);

    scene_reset(get_scene());

    QueryPerformanceFrequency(&frequency);

    RECT windowRect;
    GetClientRect(hwnd, &windowRect);
    windowCenter.x = (windowRect.right - windowRect.left) / 2;
    windowCenter.y = (windowRect.bottom - windowRect.top) / 2;
    ClientToScreen(hwnd, &windowCenter); // Convert to screen coordinates

    SetCursorPos(windowCenter.x, windowCenter.y); // Set cursor to window center
    ShowCursor(FALSE); // Hide the cursor
    SetCapture(hwnd);  // Capture the mouse input

    MSG msg = {};
    QueryPerformanceCounter(&frameStart);
    for (unsigned long long i = 0; true; i++) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) {
                return 0;
            }
        }

        QueryPerformanceCounter(&frameEnd);
        real delta_time = real_float(double(frameEnd.QuadPart - frameStart.QuadPart) / frequency.QuadPart);
        frameStart = frameEnd;

        switch (mode) {
        case Mode::EXPLORE:
            exploreMode(hwnd, delta_time);
            break;
        case Mode::PICTURE:
            pictureMode(hwnd, delta_time);
            break;
        }
    }

    return 0;
}
