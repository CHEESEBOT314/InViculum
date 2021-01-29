#include "render/render_manager.hxx"

#include "vulkan_wrapper.hxx"
#include "render/push_constants.hxx"
#include "render/vertex.hxx"
#include "resource/resource_manager.hxx"

#include <map>

namespace render::render_manager {
        namespace {
            // Simple structure to hold details and a render pipeline
            struct pipeline {
                vk::PipelineLayout layout;
                vk::Pipeline pl;
            };

            // Structure inside of an anonymous namespace to provide a 'private' storage
            struct info {
                // Storage of all pipelines in maps for easy swapping
                std::map<std::string, uint32_t> name_id_map;
                std::map<uint32_t, pipeline> id_pipeline_map;
                uint32_t next_id = 1;
                bool loaded = false;

                vk::Buffer rect_2D;
                vk::DeviceMemory rect_2D_memory;
                vk::DeviceSize* offsets = nullptr;

                push_constants current_pc;
                pipeline* current_pl = nullptr;
            };
            std::unique_ptr<info> info_p;

            /**
             * load_pipeline - Load Pipeline function loads the give pipeline from the binary files
             * @param name - name of the pipeline to be loaded
             * @param pipeline - variable to hold the returned pipeline and layout
             * @return - successful or not
             */
            bool load_pipeline(const std::string& name, pipeline& pipeline) {
                // Create shader modules from the provided files
                vk::ShaderModule vert, frag;
                if (!vulkan_wrapper::create_shader_module(vert,
                                                       resource::resource_manager::read_binary_file(name + ".vs.spv",
                                                                                                   {"shaders"})) ||
                    !vulkan_wrapper::create_shader_module(frag,
                                                       resource::resource_manager::read_binary_file(name + ".fs.spv",
                                                                                                   {"shaders"}))) {
                    vulkan_wrapper::destroy_shader_module(vert);
                    vulkan_wrapper::destroy_shader_module(frag);
                    return false;
                }
                // Add shader modules to the creation info
                vk::PipelineShaderStageCreateInfo shader_stage_create_infos[2];
                shader_stage_create_infos[0] = vk::PipelineShaderStageCreateInfo(vk::PipelineShaderStageCreateFlags(),
                                                                              vk::ShaderStageFlagBits::eVertex, vert,
                                                                              "main");
                shader_stage_create_infos[1] = vk::PipelineShaderStageCreateInfo(vk::PipelineShaderStageCreateFlags(),
                                                                              vk::ShaderStageFlagBits::eFragment, frag,
                                                                              "main");
                // Indicate the size of the push constants which store most of the information for rendering
                vk::PushConstantRange push_constant_range = {vk::ShaderStageFlagBits::eVertex,
                                                           0,
                                                           sizeof(push_constants)};

                // Tell the layout about the push constants
                vk::PipelineLayoutCreateInfo pipeline_layout_create_info = {vk::PipelineLayoutCreateFlags(),
                                                                         0,
                                                                         nullptr,
                                                                         1,
                                                                         &push_constant_range};
                // Create the pipeline layout
                if (!vulkan_wrapper::create_pipeline_layout(pipeline.layout, pipeline_layout_create_info)) {
                    vulkan_wrapper::destroy_shader_module(vert);
                    vulkan_wrapper::destroy_shader_module(frag);
                    return false;
                }

                // Enable the vertices to be sent to the shader
                vk::VertexInputBindingDescription vertex_input_binding_description = {0,
                                                                                   sizeof(vertex),
                                                                                   vk::VertexInputRate::eVertex};
                vk::VertexInputAttributeDescription vertex_input_attribute_descriptions[2];
                vertex_input_attribute_descriptions[0] = {0, 0, vk::Format::eR32G32Sfloat, (uint32_t)offsetof(vertex, pos)};
                vertex_input_attribute_descriptions[1] = {1, 0, vk::Format::eR32G32Sfloat, (uint32_t)offsetof(vertex, uv)};

                // Create the pipeline
                if (!vulkan_wrapper::create_pipeline(pipeline.pl, pipeline.layout, 2, shader_stage_create_infos, 1, &vertex_input_binding_description, 2, vertex_input_attribute_descriptions, -1.0f)) {
                    vulkan_wrapper::destroy_shader_module(vert);
                    vulkan_wrapper::destroy_shader_module(frag);
                    vulkan_wrapper::destroy_pipeline_layout(pipeline.layout);
                    return false;
                }

                // Discard the used shader modules
                vulkan_wrapper::destroy_shader_module(vert);
                vulkan_wrapper::destroy_shader_module(frag);

                // Returns true if successful
                return true;
            }
        }

        // Hook to get the aspect ratio from Vulkan without including the entire header
        float get_aspect_ratio() {
            return vulkan_wrapper::get_aspect_ratio();
        }

        /**
         * init - Init function initialises the Render Manager where it creates the vertex buffer for the 2D rectangle
         */
        void init() {
            info_p = std::make_unique<info>();
            static const std::vector<vertex> vertices_2D = {
                    {{0.0f, 0.0f}, {0.0f, 0.0f}},
                    {{1.0f, 0.0f}, {1.0f, 0.0f}},
                    {{1.0f, 1.0f}, {1.0f, 1.0f}},
                    {{0.0f, 0.0f}, {0.0f, 0.0f}},
                    {{1.0f, 1.0f}, {1.0f, 1.0f}},
                    {{0.0f, 1.0f}, {0.0f, 1.0f}}};
            vulkan_wrapper::create_vertex_buffer(info_p->rect_2D, info_p->rect_2D_memory, sizeof(vertex) * vertices_2D.size());
            vulkan_wrapper::map_vertex_buffer(info_p->rect_2D_memory, sizeof(vertex) * vertices_2D.size(), vertices_2D.data());
            info_p->offsets = new vk::DeviceSize[1]{0};
            reset_push_constants();
        }

        /**
         * create_graphics_pipline - Create Graphics Pipeline function creates a given pipeline
         * @param name - name of the pipeline
         * @return - successful or not
         */
        bool create_graphics_pipeline(const std::string& name) {
            // Add name to map with the next available id
            info_p->name_id_map.insert(std::pair<const std::string, uint32_t>(name, info_p->next_id));
            if (info_p->loaded) {
                pipeline pl;
                // Load the pipeline
                if (!load_pipeline(name, pl)) {
                    return false;
                }
                // Place pipeline into the map for easy loading
                info_p->id_pipeline_map.insert(std::pair<const uint32_t, pipeline>(info_p->next_id, pl));
            }
            info_p->next_id++;
            return true;
        }

        /**
         * get_pipeline - Get Pipeline function gets the loaded pipeline from the given name
         * @param name - pipeline to get
         * @return - pipeline id or 0 if not available
         */
        uint32_t get_pipeline(const std::string& name) {
            auto it = info_p->name_id_map.find(name);
            if (it != info_p->name_id_map.end()) {
                return it->second;
            }
            return 0;
        }

        /**
         * bind_pipeline - Bind Pipeline function binds the given pipeline from the id (via get_pipeline)
         * @param id - pipeline id
         */
        void bind_pipeline(uint32_t id) {
            if (id > 0) {
                //Find the pipeline and if present bind it
                auto it = info_p->id_pipeline_map.find(id);
                if (it != info_p->id_pipeline_map.end()) {
                    vulkan_wrapper::bind_pipeline(it->second.pl);
                    info_p->current_pl = &it->second;
                }
            }
        }
        /**
         * reset_push_constants - Reset Push Constants function resets all variables back to default,
         * this is called at the start of rendering
         */
        void reset_push_constants() {
            info_p->current_pc.p = vml::mat4::identity();
            info_p->current_pc.v = vml::mat4::identity();
            info_p->current_pc.m = vml::mat4::identity();
            info_p->current_pc.cm = vml::mat4::identity();
            info_p->current_pc.light_dir = vml::vec4();
            info_p->current_pc.light0 = vml::vec4();
            info_p->current_pc.light1 = vml::vec4();
            info_p->current_pc.is_shadow = false;
        }
        // Set the perspective matrix push constant
        void set_perspective(const vml::mat4& pers) {
            info_p->current_pc.p = pers;
        }
        // Set the view matrix push constant
        void set_view(const vml::mat4& view) {
            info_p->current_pc.v = view;
        }
        // Set the model matrix push constant
        void set_model(const vml::mat4& mode) {
            info_p->current_pc.m = mode;
        }
        // Set the colour multiplier matrix push constant
        void set_colour_mult(const vml::mat4& cm) {
            info_p->current_pc.cm = cm;
        }
        // Set the light direction vector push constant
        void set_light_dir(const vml::vec3& l) {
            info_p->current_pc.light_dir = vml::vec4(l, 0.0F);
        }
        // Set the light0 position vector push constant
        void set_light0(const vml::vec3& l) {
            info_p->current_pc.light0 = vml::vec4(l, 0.0F);
        }
        // Set the light1 position vector push constant
        void set_light1(const vml::vec3& l) {
            info_p->current_pc.light1 = vml::vec4(l, 0.0F);
        }
        // Set the is shadow boolean push constant
        void set_is_shadow(bool is) {
            info_p->current_pc.is_shadow = is;
        }

        /**
         * draw - Draw function draws the currently binded vertex buffer using the given parameters
         * @param vertex_count - Number of vertices to draw
         * @param instance_count - Number of instances to draw
         * @param first_vertex - Offset into the vertex buffer to start
         * @param first_instance - Offset into the instances to start
         */
        void draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance) {
            vulkan_wrapper::push_constants(info_p->current_pl->layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(push_constants), &info_p->current_pc);
            vulkan_wrapper::draw(vertex_count, instance_count, first_vertex, first_instance);
        }
        // Draw the 2D rectangle used for the majority of this application described as (0, 0, 0), (1, 0, 0), (1, 1, 0) and (0, 1, 0)
        void draw_rect_2D() {
            vulkan_wrapper::bind_vertex_buffers(1, &info_p->rect_2D, info_p->offsets);
            draw(6, 1, 0, 0);
        }

        /**
         * load_shaders - Load Shaders function loads all requested pipelines
         * @return successful or not
         */
        bool load_shaders() {
            for (const std::pair<const std::string, uint32_t>& nPair : info_p->name_id_map) {
                pipeline pl;
                if (!load_pipeline(nPair.first, pl)) {
                    unload_shaders();
                    return false;
                }
                info_p->id_pipeline_map.insert(std::pair<const uint32_t, pipeline>(nPair.second, pl));
            }
            return (info_p->loaded = true);
        }

        /**
         * unload_shaders - Unload Shaders function destroys all pipelines that have been loaded
         */
        void unload_shaders() {
            for (const std::pair<const uint32_t, pipeline>& pPair : info_p->id_pipeline_map) {
                vulkan_wrapper::destroy_pipeline_layout(pPair.second.layout);
                vulkan_wrapper::destroy_pipeline(pPair.second.pl);
            }
            info_p->id_pipeline_map.clear();
            info_p->loaded = false;
        }
        // Function to reload all pipelines, not used
        bool reload_shaders() {
            unload_shaders();
            return load_shaders();
        }
        /**
         * terminate - Terminate function is called when the application closes freeing up all of the memory
         */
        void terminate() {
            if (info_p->loaded) {
                unload_shaders();
            }
            delete[] info_p->offsets;
            vulkan_wrapper::destroy_vertex_buffer(info_p->rect_2D, info_p->rect_2D_memory);
            info_p->name_id_map.clear();
            info_p.reset(nullptr);
        }
    }