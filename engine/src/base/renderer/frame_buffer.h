#pragma once

namespace Yogi {

	struct FrameBufferProps
	{
		uint32_t width = 0, height = 0;
		uint32_t samples = 1;

		bool swap_chain_target = false;
	};

	class FrameBuffer
	{
	public:
		virtual ~FrameBuffer() = default;

		virtual void bind() = 0;
		virtual void unbind() = 0;

        virtual uint32_t get_color_attachment() const = 0;

		static Ref<FrameBuffer> create(const FrameBufferProps& props);
	};

}