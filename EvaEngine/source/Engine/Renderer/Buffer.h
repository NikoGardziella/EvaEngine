#pragma once
#include <Engine/Core/Core.h>


namespace Engine {

	enum class ShaderDataType
	{
		None = 0,
		Float,
		Float2,
		Float3,
		Float4,
		Mat3,
		Mat4,
		Int,
		Int2,
		Int3,
		Int4,
		Bool
	};


	static uint32_t ShaderDataTypeSize(ShaderDataType type)
	{
		switch (type)
		{
		case Engine::ShaderDataType::None:
			return 0;
		case Engine::ShaderDataType::Float:
			return 4;
		case Engine::ShaderDataType::Float2:
			return 4 * 2;
		case Engine::ShaderDataType::Float3:
			return 4 * 3;
		case Engine::ShaderDataType::Float4:
			return 4 * 4;
		case Engine::ShaderDataType::Mat3:
			return 4 * 3 * 3;
		case Engine::ShaderDataType::Mat4:
			return 4 * 4 * 4;
		case Engine::ShaderDataType::Int:
			return 4;
		case Engine::ShaderDataType::Int2:
			return 4 * 2;
		case Engine::ShaderDataType::Int3:
			return 4 * 3;
		case Engine::ShaderDataType::Int4:
			return 4 * 4;
		case Engine::ShaderDataType::Bool:
			return 1;

		}
		EE_CORE_ASSERT(false, " unkonwn ShaderDataType!");
		return 0;

	}

	struct BufferElement
	{
		std::string Name;
		uint32_t Offset;
		uint32_t Size;
		ShaderDataType Type;
		bool Normalized;

		BufferElement(ShaderDataType type, const std::string& name, bool normalized = false)
			: Name(name), Type(type), Size(ShaderDataTypeSize(type)), Offset(0), Normalized(normalized)
		{

		}

		uint32_t GetComponentCount() const
		{
			switch (Type)
			{
			case ShaderDataType::Float:   return 1;
			case ShaderDataType::Float2:  return 2;
			case ShaderDataType::Float3:  return 3;
			case ShaderDataType::Float4:  return 4;
			case ShaderDataType::Mat3:    return 3 * 3; // 3x3 matrix
			case ShaderDataType::Mat4:    return 4 * 4; // 4x4 matrix
			case ShaderDataType::Int:     return 1;
			case ShaderDataType::Int2:    return 2;
			case ShaderDataType::Int3:    return 3;
			case ShaderDataType::Int4:    return 4;
			case ShaderDataType::Bool:    return 1;
			}
			EE_CORE_ASSERT(false, "Unknown ShaderDataType!");
			return 0;
		}
	};

	class BufferLayout
	{
	public:


		BufferLayout();

		//  automatically constructs a std::initializer_list<BufferElements>
		// object which contains the values in given order. This object is passed to the constructor. 
		BufferLayout(const std::initializer_list<BufferElement>& elements)
			: m_elements(elements)
		{
			CalculateOffsetAndStride();
		}


		inline const uint32_t GetStride() const { return m_stride; }
		inline const std::vector<BufferElement>& GetElements() const { return m_elements; }

		std::vector<BufferElement>::const_iterator begin() const { return m_elements.begin(); }
		std::vector<BufferElement>::const_iterator end()	const { return m_elements.end(); }

	private:
		std::vector<BufferElement> m_elements;

		// number of bytes between the start of one element and
		// the start of the next element in a data structure. 
		uint32_t m_stride = 0;
	private:

		void CalculateOffsetAndStride();
		
	};


	class VertexBuffer
	{
	public:
		virtual ~VertexBuffer() {};

		//virtual void SetData();

		virtual void Bind() const = 0;
		virtual void UnBind() const = 0;

		virtual void SetLayout(const BufferLayout& layout) = 0;
		virtual const BufferLayout GetLayout() const = 0;

		virtual void SetData(const void* data, uint32_t size) = 0;
		virtual void SetMat4InstanceAttribute(uint32_t location) = 0;

		static Ref<VertexBuffer> Create(float* vertices, uint32_t size);
		static Ref<VertexBuffer> Create(uint32_t size);

		virtual uint32_t GetSize() const = 0;



	};

	class IndexBuffer
	{

	public:
		virtual ~IndexBuffer() {};

		virtual void Bind() const = 0;
		virtual void UnBind() const = 0;

		virtual uint32_t GetCount() const = 0;

		static  Ref<IndexBuffer> Create(uint32_t* indices, uint32_t count);
		virtual const void* GetData() const = 0;


	};

}