#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_gfx/SDL3_gfxPrimitives.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <vector>

// Global init flags
static bool SDL_inited = false;
static bool TTF_inited = false;
static bool MIX_inited = false;

// Global pointers to clean up (optional convenience)
static SDL_Window* g_window = nullptr;
static SDL_Renderer* g_renderer = nullptr;
static TTF_Font* g_font = nullptr;
static SDL_Texture* g_textTexture = nullptr;
static IMG_Animation* g_animation = nullptr;
static std::vector<SDL_Texture*> g_frameTextures;
static MIX_Track * g_mixerTrack = nullptr;
static MIX_Mixer* g_mixer = nullptr;
static MIX_Audio* g_music = nullptr;

// Standalone helper to get per-frame delay in ms (with sane default)
static int GetFrameDelayMS(const IMG_Animation* anim, int idx)
{
    if (!anim || anim->count <= 0) return 100;
    if (idx < 0 || idx >= anim->count) return 100;
    int delay = anim->delays ? anim->delays[idx] : 100;
    return delay > 0 ? delay : 100;
}

// Unified cleanup function
static void CleanUp()
{
    // Destroy textures created from animation frames
    for (SDL_Texture* t : g_frameTextures) {
        if (t) SDL_DestroyTexture(t);
    }
    g_frameTextures.clear();

    if (g_animation) {
        IMG_FreeAnimation(g_animation);
        g_animation = nullptr;
    }

    if (g_textTexture) {
        SDL_DestroyTexture(g_textTexture);
        g_textTexture = nullptr;
    }

    if (g_font) {
        TTF_CloseFont(g_font);
        g_font = nullptr;
    }

    if (g_renderer) {
        SDL_DestroyRenderer(g_renderer);
        g_renderer = nullptr;
    }

    if (g_window) {
        SDL_DestroyWindow(g_window);
        g_window = nullptr;
    }

    if (g_mixerTrack) {
        MIX_StopTrack(g_mixerTrack, MIX_TrackMSToFrames(g_mixerTrack, 1000));
    }

    if (MIX_inited) {
        MIX_Quit();
        MIX_inited = false;
    }

    if (TTF_inited) {
        TTF_Quit();
        TTF_inited = false;
    }

    if (SDL_inited) {
        SDL_Quit();
        SDL_inited = false;
    }
}

int main(int argc, char* argv[])
{
    // Init SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_inited = true;
    } else {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        CleanUp();
        return 1;
    }

    // Init TTF
    if (TTF_Init()) {
        TTF_inited = true;
    } else {
        SDL_Log("TTF_Init failed: %s", SDL_GetError());
        CleanUp();
        return 1;
    }

    if (MIX_Init()) {
        MIX_inited = true;
    } else {
        SDL_Log("MIX_Init failed: %s", SDL_GetError());
        CleanUp();
        return 1;
    }

    // Create window + renderer
    if (!SDL_CreateWindowAndRenderer("SDL3 + TTF + APNG Example", 800, 600, 0, &g_window, &g_renderer)) {
        SDL_Log("Window/Renderer creation failed: %s", SDL_GetError());
        CleanUp();
        return 1;
    }

    // Load font
    g_font = TTF_OpenFont("yahei.ttf", 64);
    if (!g_font) {
        SDL_Log("Failed to load font: %s", SDL_GetError());
        CleanUp();
        return 1;
    }

    // Create text texture
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Surface* textSurface = TTF_RenderText_Blended(g_font, "Hello SDL3! 你好!", 0, textColor);
    if (!textSurface) {
        SDL_Log("Failed to create text surface: %s", SDL_GetError());
        CleanUp();
        return 1;
    }
    g_textTexture = SDL_CreateTextureFromSurface(g_renderer, textSurface);
    float textW = static_cast<float>(textSurface->w);
    float textH = static_cast<float>(textSurface->h);
    SDL_DestroySurface(textSurface);

    g_mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
    if (g_mixer == nullptr) {
        SDL_Log("Failed to create mixer device: %s", SDL_GetError());
        CleanUp();
        return 1;
    }

    g_mixerTrack = MIX_CreateTrack(g_mixer);
    g_music = MIX_LoadAudio(g_mixer, "sound.ogg", false);
    if (not g_music) {
        SDL_Log("Failed to load music: %s", SDL_GetError());
        CleanUp();
        return 1;
    }

    MIX_SetTrackAudio(g_mixerTrack, g_music);
    MIX_PlayTrack(g_mixerTrack, NULL);

    // Load APNG animation
    g_animation = IMG_LoadAnimation("elephant.png");
    if (!g_animation) {
        SDL_Log("Failed to load APNG animation: %s", SDL_GetError());
        CleanUp();
        return 1;
    }

    // Convert frames to textures
    g_frameTextures.resize(g_animation->count, nullptr);
    for (int i = 0; i < g_animation->count; ++i) {
        SDL_Surface* frameSurface = g_animation->frames[i];
        if (!frameSurface) continue;
        g_frameTextures[i] = SDL_CreateTextureFromSurface(g_renderer, frameSurface);
        if (!g_frameTextures[i]) {
            SDL_Log("Failed to create texture for frame %d: %s", i, SDL_GetError());
        }
    }

    // Layout
    SDL_FRect textRect = { (800 - textW) / 2.0f, (600 - textH) / 2.0f, textW, textH };
    int frameW = (g_animation->count > 0 && g_animation->frames[0]) ? g_animation->frames[0]->w : 0;
    int frameH = (g_animation->count > 0 && g_animation->frames[0]) ? g_animation->frames[0]->h : 0;
    SDL_FRect apngRect = { 20.0f, 20.0f, static_cast<float>(frameW), static_cast<float>(frameH) };

    // Animation state
    bool running = true;
    SDL_Event event;
    int currentFrame = 0;
    Uint64 frameStartMS = SDL_GetTicks();

    // Main loop
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        Uint64 nowMS = SDL_GetTicks();
        int currentDelay = GetFrameDelayMS(g_animation, currentFrame);
        if (nowMS - frameStartMS >= static_cast<Uint64>(currentDelay)) {
            currentFrame = (currentFrame + 1) % g_animation->count;
            frameStartMS = nowMS;
        }

        SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
        SDL_RenderClear(g_renderer);

        SDL_Texture* apngTex = (currentFrame >= 0 && currentFrame < (int)g_frameTextures.size())
                                 ? g_frameTextures[currentFrame]
                                 : nullptr;
        if (apngTex) {
            SDL_RenderTexture(g_renderer, apngTex, nullptr, &apngRect);
        }

        SDL_RenderTexture(g_renderer, g_textTexture, nullptr, &textRect);
        boxRGBA(g_renderer, 0, 0, 20, 20, 0, 0, 255, 255);

        SDL_RenderPresent(g_renderer);
    }

    CleanUp();
    return 0;
}
