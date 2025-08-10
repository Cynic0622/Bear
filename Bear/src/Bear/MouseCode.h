#pragma once
namespace Bear
{
	using MouseCode = uint8_t;

	namespace Mouse
	{
		enum : MouseCode
		{
			MouseButton0 = 0x00, // Left button
			MouseButton1 = 0x01, // Right button
			MouseButton2 = 0x04, // Middle button
			MouseButton3 = 0x08, // Button 4
			MouseButton4 = 0x10, // Button 5
			MouseButton5 = 0x20, // Button 6
			MouseButton6 = 0x40, // Button 7
			MouseButton7 = 0x80, // Button 8
			MouseButtonLast = MouseButton7,
			Left = MouseButton0,
			Right = MouseButton1,
			Middle = MouseButton2
		};
	}
}
