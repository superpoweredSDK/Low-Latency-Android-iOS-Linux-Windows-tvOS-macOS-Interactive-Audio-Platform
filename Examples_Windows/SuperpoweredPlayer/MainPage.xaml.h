//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"

namespace SuperpoweredPlayer
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	public ref class MainPage sealed
	{
	public:
		MainPage();
		void Toggle(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
	private:
		void OnTick(Platform::Object^ sender, Platform::Object^ e);
	};
}
