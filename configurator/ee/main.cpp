#include <iopcontrol.h>
#include <iopheap.h>
#include <kernel.h>
#include <loadfile.h>
#include <sbv_patches.h>
#include <sifrpc.h>
#include <stdio.h>

#include <ps2plus_common.h>

#include "irx_builtins.h"
#include "libps2plman.h"

#include "ui/custom_font.h"
#include "ui/drawing/drawing.h"
#include "ui/gfx.h"
#include "ui/pad.h"
#include "ui/widgets/widget.h"

typedef struct {
    int port;
    bool connected;
    struct {
        uint16_t firmware;
        char microcontroller[33];
        uint16_t configuration;
    } versions;
    struct {
        uint8_t joystick_axis_range_remapping_values[12];
        bool joystick_digital_mode;
        bool global_button_swap;
    } configuration;
} configurator_ps2plus_controller;

struct {
    configurator_ps2plus_controller controllers[2];
} configurator_state;

bool is_reading_ps2_state = false;

bool IsGamepadCaptured() {
    return !(ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_NavEnableGamepad);
}

void BeginCaptureGamepad() {
    ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;
    ImGui::GetIO().MouseDrawCursor = false;
}

void EndCaptureGamepad() {
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
}

// Helper to display a little (?) mark which shows a tooltip when hovered.
// In your own code you may want to display an actual icon if you are using a merged icon fonts (see docs/FONTS.md)
static void HelpMarker(const char* desc)
{
    ImGui::PushID(desc);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 0));
    ImGui::SmallButton("?");
    ImGui::PopStyleVar();
    ImGui::PopID();
    if ((ImGui::GetIO().MouseDrawCursor && ImGui::IsItemHovered()) || (!ImGui::GetIO().MouseDrawCursor && ImGui::IsItemFocused())) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 25.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void app_ps2plus_ports_display() {
    // Information section
    if (ImGui::BeginTable("PS2+ Ports#Table", 2)) {
        ImGui::TableNextRow();
        for (int i = 0; i < sizeof(configurator_state.controllers) / sizeof(configurator_ps2plus_controller); i++) {
            configurator_ps2plus_controller *controller = &configurator_state.controllers[i];

            ImGui::TableSetColumnIndex(i);
            
            // Draw a custom icon
            const ImVec2 p = ImGui::GetCursorScreenPos();
            const float icon_size = ImGui::GetFontSize();
            const ImVec2 icon_center = ImVec2(p.x + icon_size/2, p.y + icon_size/2);
            ImDrawList *draw_list = ImGui::GetWindowDrawList();
            if (controller->connected) {
                draw_list->AddCircleFilled(icon_center, icon_size/2, IM_COL32(0x00, 0xFF, 0x00, 0x80));
            } else {
                draw_list->AddCircle(icon_center, icon_size/2, IM_COL32(0xFF, 0xFF, 0xFF, 0x80));
            }
            ImGui::Dummy(ImVec2(icon_size, icon_size));
            ImGui::SameLine();
            
            ImGui::BeginGroup();
            {
                if (controller->connected) {
                    ImGui::TextColored(ImVec4(0, 1, 0, 1.0), "Connected to PS2+");
                    ImGui::SetWindowFontScale(0.75);
                    ImGui::Text("Port %d", i + 1);
                    ImGui::SetWindowFontScale(1.0);
                } else {
                    ImGui::TextDisabled("No PS2+ found");
                    ImGui::SetWindowFontScale(0.75);
                    ImGui::TextDisabled("Port %d", i + 1);
                    ImGui::SetWindowFontScale(1.0);
                }
            }
            ImGui::EndGroup();
        }
        ImGui::EndTable();
    }
}

void app_section_information(const ImGuiIO &io, PadStatus *pad_status) {
    // Version data
    if (configurator_state.controllers[0].connected) {
        configurator_ps2plus_controller *controller = &configurator_state.controllers[0];

        if (ImGui::BeginTable("PS2+ Version Number", 2, ImGuiTableFlags_Borders)) {
            ImGui::TableSetupColumn("Version Number", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            ImGui::TableNextRow();
            ImGui::TableNextColumn(); 
            ImGui::Text("Firmware"); ImGui::SameLine();
            HelpMarker("Version of the PS2+'s firmware.");
            ImGui::TableNextColumn(); 
            ImGui::TextDisabled("%d", controller->versions.firmware);
            
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); 
            ImGui::Text("Microcontroller"); ImGui::SameLine();
            HelpMarker(
                "Name of the the PS2+'s microcontroller. "
                "Firmware updates must be for this same microcontroller.");
            ImGui::TableNextColumn(); 
            ImGui::TextDisabled("%s", controller->versions.microcontroller);
            
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); 
            ImGui::Text("Configuration"); ImGui::SameLine();
            HelpMarker(
                "Version of the PS2+'s internal configuration format. "
                "Firmware updates that change the configuration version will "
                "reset the PS2+'s configuration to avoid "
                "incompatibilities.");
            ImGui::TableNextColumn(); 
            ImGui::TextDisabled("%d", controller->versions.configuration);

            ImGui::EndTable();
        }
    }
    ImGui::Separator();
    
    // Gamepad tester
    ImGui::Indent(50);
    ImGui::Widgets::GamePadVisualizer(&pad_status->pad, ImGui::GetWindowWidth() * 0.73, ImGui::GetWindowHeight() * 0.40);
}

void app_section_configuration(ImGuiIO &io, PadStatus *pad_status) {
    configurator_ps2plus_controller *controller = &configurator_state.controllers[0];

    static bool global_button_swap = controller->configuration.global_button_swap;
    static bool joystick_digital_mode = controller->configuration.joystick_digital_mode;
    static uint8_t joystick_axis_range_remappings[12] = { 0, 127, 255, 0, 127, 255, 0, 127, 255, 0, 127, 255 };

    if (ImGui::TreeNodeEx("General", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("##global_button_swap", &global_button_swap); 
        ImGui::SameLine(); ImGui::Text("Globally swap the");
        ImGui::SameLine(); ImGui::Widgets::GamePadIcon(ImGui::Widgets::WidgetGamePadIconType_Cross);
        ImGui::SameLine(); ImGui::Text("and");
        ImGui::SameLine(); ImGui::Widgets::GamePadIcon(ImGui::Widgets::WidgetGamePadIconType_Circle);
        ImGui::SameLine(); ImGui::Text("buttons");        

        ImGui::TreePop();
        ImGui::Separator();
    }

    if (ImGui::TreeNodeEx("Joysticks", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::TreeNodeEx("General##Joysticks", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("##joystick_digital_mode", &joystick_digital_mode); 
            ImGui::SameLine(); ImGui::TextWrapped(
                "Use the left and right joysticks to simulate the D-pad "
                "and the face buttons when in digital mode");

            ImGui::TreePop();
            ImGui::Separator();
        }

        if (ImGui::TreeNodeEx("Axis Range Remapping##Joysticks", ImGuiTreeNodeFlags_DefaultOpen)) {
            static bool is_calibrating = false;

            ImGui::BeginGroup();
            {
                const ImVec2 p = ImGui::GetCursorScreenPos();
                const float joystick_size = 90.f;
                ImDrawList *draw_list = ImGui::GetWindowDrawList();
                draw_controller_joystick(
                    draw_list, 
                    ImVec2(p.x + joystick_size/2, p.y + joystick_size/2), 
                    joystick_size / 2, is_calibrating, joystick_axis_range_remappings[1], joystick_axis_range_remappings[4]);

                draw_list->AddRectFilled(
                    ImVec2(p.x + joystick_size/2 - joystick_size/2*(1 - (float)joystick_axis_range_remappings[0]/128), p.y + joystick_size/2 - joystick_size/2*(1 - (float)joystick_axis_range_remappings[3]/128)),
                    ImVec2(p.x + joystick_size/2 + joystick_size/2*((float)joystick_axis_range_remappings[2]/128 - 1), p.y + joystick_size/2 + joystick_size/2*((float)joystick_axis_range_remappings[5]/128 - 1)),
                    IM_COL32(0x55, 0x55, 0xAA, 0x60), 5.0f);

                if (is_calibrating) {
                    draw_list->AddCircleFilled(
                        ImVec2(p.x + joystick_size*(pad_status->pad.ljoy_h/255.f), p.y + joystick_size*(pad_status->pad.ljoy_v/255.f)),
                        2.0f, IM_COL32(0x55, 0x55, 0xAA, 0xC0));
                }

                ImGui::Dummy(ImVec2(joystick_size, joystick_size));
            }
            ImGui::EndGroup();
            ImGui::SameLine();
            ImGui::BeginGroup();
            {
                const ImVec2 p = ImGui::GetCursorScreenPos();
                const float joystick_size = 90.f;
                ImDrawList *draw_list = ImGui::GetWindowDrawList();
                draw_controller_joystick(
                    draw_list, 
                    ImVec2(p.x + joystick_size/2, p.y + joystick_size/2), 
                    joystick_size / 2, is_calibrating, joystick_axis_range_remappings[7], joystick_axis_range_remappings[10]);
                
                draw_list->AddRectFilled(
                    ImVec2(p.x + joystick_size/2 - joystick_size/2*(1 - (float)joystick_axis_range_remappings[6]/128), p.y + joystick_size/2 - joystick_size/2*(1 - (float)joystick_axis_range_remappings[9]/128)),
                    ImVec2(p.x + joystick_size/2 + joystick_size/2*((float)joystick_axis_range_remappings[8]/128 - 1), p.y + joystick_size/2 + joystick_size/2*((float)joystick_axis_range_remappings[11]/128 - 1)),
                    IM_COL32(0x55, 0x55, 0xAA, 0x60), 5.0f);

                if (is_calibrating) {
                    draw_list->AddCircleFilled(
                        ImVec2(p.x + joystick_size*(pad_status->pad.rjoy_h/255.f), p.y + joystick_size*(pad_status->pad.rjoy_v/255.f)),
                        2.0f, IM_COL32(0x55, 0x55, 0xAA, 0xC0));
                }

                ImGui::Dummy(ImVec2(joystick_size, joystick_size));
            }
            ImGui::EndGroup();
            ImGui::SameLine();
            ImGui::BeginGroup();
            {
                ImGui::BeginDisabled(is_calibrating);
                if (ImGui::Button("Calibrate") && !is_calibrating) {
                    ps2plman_disable_enable_configuration(false, NULL);
                    is_calibrating = true;
                    BeginCaptureGamepad();

                    for (int js = 0; js < 2; js++) {
                        for (int axis = 0; axis < 2; axis++) {
                            joystick_axis_range_remappings[js*6 + axis*3 + 0] = 111;
                            joystick_axis_range_remappings[js*6 + axis*3 + 1] = 127;
                            joystick_axis_range_remappings[js*6 + axis*3 + 2] = 143;
                        }
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Reset##JoystickAxisRangeMapping")) {
                    for (int js = 0; js < 2; js++) {
                        for (int axis = 0; axis < 2; axis++) {
                            joystick_axis_range_remappings[js*6 + axis*3 + 0] = 0;
                            joystick_axis_range_remappings[js*6 + axis*3 + 1] = 127;
                            joystick_axis_range_remappings[js*6 + axis*3 + 2] = 255;
                        }
                    }
                }
                ImGui::EndDisabled();

                if (is_calibrating) {
                    ImGui::Widgets::GamePadIcon(ImGui::Widgets::WidgetGamePadIconType_Square); ImGui::SameLine();
                        ImGui::Text("Update centers");

                    ImGui::Widgets::GamePadIcon(ImGui::Widgets::WidgetGamePadIconType_Circle); ImGui::SameLine();
                        ImGui::Text("Finish calibration");
                }

                if (is_calibrating && pad_status->buttonsNew & PAD_SQUARE) {
                    joystick_axis_range_remappings[1] = pad_status->pad.ljoy_h;
                    joystick_axis_range_remappings[4] = pad_status->pad.ljoy_v;
                    joystick_axis_range_remappings[7] = pad_status->pad.rjoy_h;
                    joystick_axis_range_remappings[10] = pad_status->pad.rjoy_v;
                }

                if (is_calibrating && pad_status->buttonsNew & PAD_CIRCLE) {
                    is_calibrating = false;
                    EndCaptureGamepad();
                    ps2plman_disable_enable_configuration(true, NULL);
                }

                if (is_calibrating) {
                    if (pad_status->pad.ljoy_h < joystick_axis_range_remappings[0])
                        joystick_axis_range_remappings[0] = pad_status->pad.ljoy_h;
                    if (pad_status->pad.ljoy_h > joystick_axis_range_remappings[2])
                        joystick_axis_range_remappings[2] = pad_status->pad.ljoy_h;
                    if (pad_status->pad.ljoy_v < joystick_axis_range_remappings[3])
                        joystick_axis_range_remappings[3] = pad_status->pad.ljoy_v;
                    if (pad_status->pad.ljoy_v > joystick_axis_range_remappings[5])
                        joystick_axis_range_remappings[5] = pad_status->pad.ljoy_v;
                        
                    if (pad_status->pad.rjoy_h < joystick_axis_range_remappings[6])
                        joystick_axis_range_remappings[6] = pad_status->pad.rjoy_h;
                    if (pad_status->pad.rjoy_h > joystick_axis_range_remappings[8])
                        joystick_axis_range_remappings[8] = pad_status->pad.rjoy_h;
                    if (pad_status->pad.rjoy_v < joystick_axis_range_remappings[9])
                        joystick_axis_range_remappings[9] = pad_status->pad.rjoy_v;
                    if (pad_status->pad.rjoy_v > joystick_axis_range_remappings[11])
                        joystick_axis_range_remappings[11] = pad_status->pad.rjoy_v;
                }
            }
            ImGui::EndGroup();

            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(2, 2));
            ImGui::BeginDisabled();
            if (ImGui::BeginTable("Joystick Axis Range Remapping", 8)) {
                uint8_t first_min = 0, first_max = 127, second_min = 128, second_max = 255;

                ImGui::TableSetupColumn("##LX-Label", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("##LX-Value", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("##LY-Label", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("##LY-Value", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("##RX-Label", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("##RX-Value", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("##RY-Label", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("##RY-Value", ImGuiTableColumnFlags_WidthStretch);

                ImGui::TableNextRow();
                ImGui::TableNextColumn(); ImGui::Text("LX-");
                ImGui::TableNextColumn(); ImGui::DragScalar("##LX-", ImGuiDataType_U8, &joystick_axis_range_remappings[0], 1.0F, &first_min, &first_max);
                ImGui::TableNextColumn(); ImGui::Text("LY-");
                ImGui::TableNextColumn(); ImGui::DragScalar("##LY-", ImGuiDataType_U8, &joystick_axis_range_remappings[3], 1.0F, &first_min, &first_max);
                ImGui::TableNextColumn(); ImGui::Text("RX-");
                ImGui::TableNextColumn(); ImGui::DragScalar("##RX-", ImGuiDataType_U8, &joystick_axis_range_remappings[6], 1.0F, &first_min, &first_max);
                ImGui::TableNextColumn(); ImGui::Text("RY-");
                ImGui::TableNextColumn(); ImGui::DragScalar("##RY-", ImGuiDataType_U8, &joystick_axis_range_remappings[9], 1.0F, &first_min, &first_max);
                
                ImGui::TableNextRow();
                ImGui::TableNextColumn(); ImGui::Text("LX");
                ImGui::TableNextColumn(); ImGui::DragScalar("##LX", ImGuiDataType_U8, &joystick_axis_range_remappings[1], 1.0F, &joystick_axis_range_remappings[0], &joystick_axis_range_remappings[2]);
                ImGui::TableNextColumn(); ImGui::Text("LY");
                ImGui::TableNextColumn(); ImGui::DragScalar("##LY", ImGuiDataType_U8, &joystick_axis_range_remappings[4], 1.0F, &joystick_axis_range_remappings[3], &joystick_axis_range_remappings[5]);
                ImGui::TableNextColumn(); ImGui::Text("RX");
                ImGui::TableNextColumn(); ImGui::DragScalar("##RX", ImGuiDataType_U8, &joystick_axis_range_remappings[7], 1.0F, &joystick_axis_range_remappings[6], &joystick_axis_range_remappings[8]);
                ImGui::TableNextColumn(); ImGui::Text("RY");
                ImGui::TableNextColumn(); ImGui::DragScalar("##RY", ImGuiDataType_U8, &joystick_axis_range_remappings[10], 1.0F, &joystick_axis_range_remappings[9], &joystick_axis_range_remappings[11]);
                
                ImGui::TableNextRow();
                ImGui::TableNextColumn(); ImGui::Text("LX+");
                ImGui::TableNextColumn(); ImGui::DragScalar("##LX+", ImGuiDataType_U8, &joystick_axis_range_remappings[2], 1.0F, &second_min, &second_max);
                ImGui::TableNextColumn(); ImGui::Text("LY+");
                ImGui::TableNextColumn(); ImGui::DragScalar("##LY+", ImGuiDataType_U8, &joystick_axis_range_remappings[5], 1.0F, &second_min, &second_max);
                ImGui::TableNextColumn(); ImGui::Text("RX+");
                ImGui::TableNextColumn(); ImGui::DragScalar("##RX+", ImGuiDataType_U8, &joystick_axis_range_remappings[8], 1.0F, &second_min, &second_max);
                ImGui::TableNextColumn(); ImGui::Text("RY+");
                ImGui::TableNextColumn(); ImGui::DragScalar("##RY+", ImGuiDataType_U8, &joystick_axis_range_remappings[11], 1.0F, &second_min, &second_max);

                ImGui::EndTable();
            }
            ImGui::EndDisabled();
            ImGui::PopStyleVar();

            ImGui::TreePop();
            ImGui::Separator();
        }

        ImGui::TreePop();
    }
}

void app_section_update(ImGuiIO &io, PadStatus *pad_status) {
    // Style editor section (using ImGui's built-in editor window)
    ImGui::ShowStyleEditor();
}

void app_section_about(const ImGuiIO &io, PadStatus *pad_status) {
    
}

void demo_paned(ImGuiIO &io, PadStatus *pad_status) {
    // Whether the user is selecting a section in the left pane
    static bool is_selecting_section = false;

    // Actively displayed section content
    static int selected_content = 0;

    // Full screen "window" that can't be moved or resized
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 10));
    ImGui::Begin("PS2+ Configurator", NULL,
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    {
        // PushStyleVar in previous lines was used to increase the title-bar height
        ImGui::PopStyleVar();

        if (is_selecting_section) {
            // Only allow the left pane to have focus while the user selects
            ImGui::SetNextWindowFocus();
        }
        ImGui::BeginChild("List", ImVec2(150, 0), true);
        {
            if (ImGui::Selectable("Information", !is_selecting_section && selected_content == 0, 0, ImVec2(0, 25))) {
                selected_content = 0;
                is_selecting_section = false;
            }
            if (ImGui::Selectable("Configuration", !is_selecting_section && selected_content == 1, 0, ImVec2(0, 25))) {
                selected_content = 1;
                is_selecting_section = false;
            }
            if (ImGui::Selectable("Firmware", !is_selecting_section && selected_content == 2, 0, ImVec2(0, 25))) {
                selected_content = 2;
                is_selecting_section = false;
            }
            if (ImGui::Selectable("About", !is_selecting_section && selected_content == 3, 0, ImVec2(0, 25))) {
                selected_content = 3;
                is_selecting_section = false;
            }
        }
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginGroup();
        {
            // Content pane will retain focus if it is selected
            if (selected_content >= 0 && !is_selecting_section) {
                ImGui::SetNextWindowFocus();
            }

            app_ps2plus_ports_display();

            // Content pane has full height, less the height of a single frame which will be used to
            // display contextual button controls
            ImGui::BeginDisabled(is_selecting_section);
            ImGui::BeginChild("ChildSection", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true);
            {
                if (selected_content == 0) {
                    app_section_information(io, pad_status);
                }

                if (selected_content == 1) {
                    app_section_configuration(io, pad_status);
                }

                if (selected_content == 2) {
                    app_section_update(io, pad_status);
                }
                
                if (selected_content == 3) {
                    app_section_about(io, pad_status);
                }

                // Allow the user to change sections using the triangle button
                if (!IsGamepadCaptured() && pad_status->buttonsNew & PAD_TRIANGLE) {
                    is_selecting_section = !is_selecting_section;
                }

                // Dim this content section window if the user is selecting a different section
                if (is_selecting_section) {
                    ImGui::Widgets::WindowOverlay(0.6f);
                }
            }
            ImGui::EndChild();
            ImGui::EndDisabled();

            // Bottom row with control information
            {
                ImGui::Widgets::GamePadIcon(ImGui::Widgets::WidgetGamePadIconType_Triangle);
                ImGui::SameLine();
                ImGui::Text("Change Section");
                ImGui::SameLine();
            }
        }
        ImGui::EndGroup();
    }
    ImGui::End();
}

static void load_modules(void) {
    int ret;

    //Reboot IOP
    SifInitRpc(0);
    while(!SifIopReset("", 0)){};
    while(!SifIopSync()){};
        
    //Initialize SIF services
    SifInitRpc(0);
    SifLoadFileInit();
    SifInitIopHeap();

    // Patch is required to load modules from memory
    sbv_patch_enable_lmb();
    
    printf("Loading padman module (system) - \n");
    ret = SifLoadModule("rom0:SIO2MAN", 0, NULL);
    printf("[%d] returned\n", ret);

    printf("Loading padman module (system) - \n");
    ret = SifLoadModule("rom0:PADMAN", 0, NULL);
    printf("[%d] returned\n", ret);

    printf("Loading ps2plman module (builtin) - (%x,%d) ", (unsigned int)ps2plman_irx, size_ps2plman_irx);
    SifExecModuleBuffer(ps2plman_irx, size_ps2plman_irx, 0, NULL, &ret);
    printf("[%d] returned\n", ret);
}

void update_controllers() {
    int ret;
    bool connected = true;

    if ((ret = ps2plman_get_firmware_version(&configurator_state.controllers[0].versions.firmware)) != PS2PLMAN_RET_OK) {
        printf("Error retrieving PS2+ firmware version: %d\n", ret);
        connected = false;
    }

    if ((ret = ps2plman_get_microcontroller_version(configurator_state.controllers[0].versions.microcontroller, NULL, sizeof(configurator_state.controllers[0].versions.microcontroller) - 1)) != PS2PLMAN_RET_OK) {
        printf("Error retrieving PS2+ microcontroller version: %d\n", ret);
        connected = false;
    }

    if ((ret = ps2plman_get_configuration_version(&configurator_state.controllers[0].versions.configuration)) != PS2PLMAN_RET_OK) {
        printf("Error retrieving PS2+ configuration version: %d\n", ret);
        connected = false;
    }
    
    configurator_state.controllers[0].connected = connected;
}

int main(int argc, char **argv) {
    int ret;
    
    load_modules();
    pad_init();
    ps2plman_init();
    // update_controllers();

    // Setup the graphics and ImGui systems
    bool hires = false;
    bool use_texture_manager = true;
    GSGLOBAL *global = gfx_init(hires, use_texture_manager);
    gfx_imgui_init(global);
    ImGuiIO &io = ImGui::GetIO();

    // Rendering data!
    PadStatus pad_status;

    while (1) {
        gfx_render_begin(global, hires, use_texture_manager);
        gfx_render_clear(global, GS_SETREG_RGBA(0x30, 0x30, 0x40, 0x80));

        pad_get_status(&pad_status);
        
        demo_paned(io, &pad_status);
        gfx_render_end(global, hires, use_texture_manager);
    }

    return 0;
}