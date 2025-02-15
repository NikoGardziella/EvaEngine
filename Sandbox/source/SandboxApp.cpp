#include "Engine.h"
#include "Engine/Core/Layer.h"
#include "Engine/Core/EntryPoint.h"

#include <imgui/imgui.h>
#include <glm/ext/matrix_transform.hpp>

#include <Engine/Platform/OpenGl/OpenGLShader.h>
#include <glm/gtc/type_ptr.hpp>

#include "Sandbox2D.h"

class ExampleLayer : public Engine::Layer
{
	public: ExampleLayer() :
		Layer("Example"),
		m_orthoCameraController(1280.0f / 720.0f, true),
		m_squarePosition({1.0f})
	{
		m_vertexArray = Engine::VertexArray::Create();




		float vertices[3 * 7] = {
			-0.5f, -0.5f, 0.0f,	1.0f, 1.0f, 1.0f, 1.0f,
			0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			0.0f, 0.5f, 0.0f, 1.0f, 1.0f, 0.1f, 1.0f
		};


		Engine::Ref<Engine::VertexBuffer> vertexBuffer = Engine::VertexBuffer::Create(vertices, sizeof(vertices));


		Engine::BufferLayout layout = {
			{ Engine::ShaderDataType::Float3, "a_position" },
			{ Engine::ShaderDataType::Float4, "a_color" }

		};

		vertexBuffer->SetLayout(layout);
		m_vertexArray->AddVertexBuffer(vertexBuffer);



		uint32_t indicies[3] = { 0, 1 ,2 };
		Engine::Ref<Engine::IndexBuffer> indexBuffer = Engine::IndexBuffer::Create(indicies, sizeof(indicies) / sizeof(uint32_t));;

		m_vertexArray->SetIndexBuffer(indexBuffer);


		m_squareVertexArray = Engine::VertexArray::Create();

		float squareVertices[5 * 4] = {
			-0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
			 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 
			 0.5f,  0.5f, 0.0f, 1.0f, 1.0f, 
			-0.5f,  0.5f, 0.0f, 0.0f, 1.0f
		};
		Engine::Ref<Engine::VertexBuffer> squareVertexBuffer = Engine::VertexBuffer::Create(squareVertices, sizeof(squareVertices));


		squareVertexBuffer->SetLayout({
			{ Engine::ShaderDataType::Float3, "a_position" },
			{ Engine::ShaderDataType::Float2, "a_texCoord" }

			});
		m_squareVertexArray->AddVertexBuffer(squareVertexBuffer);

		uint32_t squareIndicies[6] = { 0, 1 ,2, 2, 3,0 };

		Engine::Ref<Engine::IndexBuffer> squareVIndexBuffer = Engine::IndexBuffer::Create(squareIndicies, sizeof(squareIndicies) / sizeof(uint32_t));

		m_squareVertexArray->SetIndexBuffer(squareVIndexBuffer);


		

		auto textureShader = m_shaderLibrary.LoadShader("assets/shaders/Texture.glsl");


		std::string blueColorShadervertexSource = R"(
			#version 330 core
			
			layout(location = 0) in vec3 a_position;

			uniform mat4 u_viewProjection;
			uniform mat4 u_transform;

			out vec3 v_position;

			void main()
			{
				v_position = a_position;
				gl_Position = u_viewProjection * u_transform * vec4(a_position, 1.0);
			}
			

		)";

		std::string blueColorShaderfragmentSource = R"(
			#version 330 core
			
			layout(location = 0) out vec4 color;

			in vec3 v_position;

			uniform vec3 u_color;

			void main()
			{
				color = vec4(u_color, 1.0);
			}
			

		)";
		m_blueShader = Engine::Shader::Create("VertexColorTriangle", blueColorShadervertexSource, blueColorShaderfragmentSource);



		std::string textureShaderVertexSource = R"(
			#version 330 core
			
			layout(location = 0) in vec3 a_position;
			layout(location = 1) in vec2 a_texCoord;


			uniform mat4 u_viewProjection;
			uniform mat4 u_transform;

			out vec2 v_texCoord;			

			void main()
			{
				v_texCoord = a_texCoord;
				gl_Position = u_viewProjection * u_transform * vec4(a_position, 1.0);
			}
			

		)";

		std::string textureShaderFragmentSource = R"(
			#version 330 core
			
			layout(location = 0) out vec4 color;

			in vec2 v_texCoord;

			uniform sampler2D u_texture;

			void main()
			{
				color = texture(u_texture, v_texCoord);
				
			}
			

		)";

		m_flatColorShader = Engine::Shader::Create("textureShader", textureShaderVertexSource, textureShaderFragmentSource);


		m_texture = Engine::Texture2D::Create("assets/textures/chess_board.png");
		m_testTexture = Engine::Texture2D::Create("assets/textures/ee_logo1.png");

		std::dynamic_pointer_cast<Engine::OpenGLShader>(textureShader)->Bind();
		std::dynamic_pointer_cast<Engine::OpenGLShader>(textureShader)->UploadUniformInt("u_texture", 0);
	}

	void OnUpdate(Engine::Timestep timestep) override
	{
		/// ************** FPS ***********
		if (m_fpsTimer > 2.0f)
		{
			float fps = 1.0f / timestep;
			EE_TRACE("FPS: {0}", fps);
			m_fpsTimer = 0.0f;
		}
		m_fpsTimer += timestep;

		m_orthoCameraController.OnUpdate(timestep);
		


		// *********** OBJECT TRANSFORM ***************
		if (Engine::Input::IsKeyPressed(EE_KEY_A))
		{
			m_squarePosition.x -= 10.0f * timestep.GetSeconds();

		}
		else if (Engine::Input::IsKeyPressed(EE_KEY_D))
		{
			m_squarePosition.x += 10.0f * timestep.GetSeconds();

		}
		if (Engine::Input::IsKeyPressed(EE_KEY_W))
		{
			m_squarePosition.y += 10.0f * timestep.GetSeconds();

		}
		else if (Engine::Input::IsKeyPressed(EE_KEY_S))
		{
			m_squarePosition.y -= 10.0f * timestep.GetSeconds();

		}

		
		//*************** RENDER ************************

		Engine::RenderCommand::SetClearColor({ 0.2f, 0, 0.2f, 1 });
		Engine::RenderCommand::Clear();

		Engine::Renderer::BeginScene(m_orthoCameraController.GetCamera());


		//Engine::MaterialRef material = new Engine::Material(m_flatColorShader);

		// Define the number of rows and columns for the grid
		const int gridRows = 20;
		const int gridCols = 20;

		// Define the size and spacing of the squares
		const float squareSize = 0.1f; // Size of each square
		const float spacing = 0.1f;   // Spacing between squares

		std::dynamic_pointer_cast<Engine::OpenGLShader>(m_blueShader)->Bind();
		std::dynamic_pointer_cast<Engine::OpenGLShader>(m_blueShader)->UploadUniformFloat3("u_color", m_squareColor);

		for (int row = 0; row < gridRows; ++row)
		{
			for (int col = 0; col < gridCols; ++col)
			{
				// Calculate the position for each square
				glm::vec3 squarePosition(
					col * (squareSize + spacing), // X position
					row * (squareSize + spacing), // Y position
					0.0f                          // Z position
				);

				// Create the transformation matrix for the square
				glm::mat4 transform = glm::translate(glm::mat4(1.0f), squarePosition) *
					glm::scale(glm::mat4(1.0f), glm::vec3(squareSize));

				Engine::Renderer::Submit(m_squareVertexArray, m_blueShader, transform);
			}
		}

		auto textureShader = m_shaderLibrary.GetShader("Texture.glsl");

		m_texture->Bind();
		Engine::Renderer::Submit(m_squareVertexArray, textureShader, glm::scale(glm::mat4(1.0f), glm::vec3(1.5f)));

		m_testTexture->Bind();
		Engine::Renderer::Submit(m_squareVertexArray, m_flatColorShader, glm::scale(glm::mat4(1.0f), glm::vec3(1.5f)));



		Engine::Renderer::EndScene();


		// Renderer::Flush();
	
	}

	void OnEvent(Engine::Event& event) override
	{
		//Engine::EventDispatcher dispatcher(event);
		//dispatcher.Dispatch<Engine::KeyPressedEvent>(EE_BIND_EVENT_FN(ExampleLayer::OnKeyPressedEvent));
		m_orthoCameraController.OnEvent(event);

	

	}

	bool OnKeyPressedEvent(Engine::KeyPressedEvent& event)
	{
		
		return false;
	}

	

	virtual void OnImGuiRender() override
	{
	
		ImGui::Begin("Settings");

		ImGui::ColorEdit3("Square color", glm::value_ptr(m_squareColor));

		ImGui::End();
	}




private:

	Engine::ShaderLibrary m_shaderLibrary;
	Engine::Ref<Engine::Shader> m_flatColorShader;
	Engine::Ref<Engine::Shader> m_blueShader;

	Engine::Ref<Engine::Texture> m_texture;
	Engine::Ref<Engine::Texture> m_testTexture;


	Engine::Ref<Engine::VertexArray> m_vertexArray;


	Engine::Ref<Engine::VertexArray> m_squareVertexArray;

	Engine::OrthographicCameraController m_orthoCameraController;


	glm::vec3 m_squarePosition;
	glm::vec3 m_squareColor = { 0.2f, 0.3f, 0.8f };


	float m_fpsTimer = 2.0f;
};

class Sandbox : public Engine::Application
{
public:
	Sandbox()
	{
		//PushLayer(new ExampleLayer());
		PushLayer(new Sandbox2D());
	}
	~Sandbox()
	{

	}

};


Engine::Application* Engine::CreateApplication()
{
	return new Sandbox;
} 