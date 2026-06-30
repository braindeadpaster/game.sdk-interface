#pragma once

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"
#include <vector>
#include <unordered_map>

inline bool init_fonts = false;

static const ImWchar default_ranges[ ] = {
	0x0020, 0x00FF,
	0x0300, 0x036F,
	0x2000, 0x206F,
	0x4E00, 0x9FFF,
	0
};

class c_font {
private:
	const char* path = nullptr;
	unsigned char* data = nullptr;
	size_t data_size = 0;
	std::unordered_map< float, ImFont* > fonts;
	ImFontConfig config;
	std::vector< ImWchar > combined_ranges;
public:
	std::vector< float > should_init = { };

	c_font( ) {
		set_ranges( default_ranges );
	}

	~c_font( ) {
		if ( data ) {
			delete[] data;
			data = nullptr;
		}
	}

	void init( std::vector< float > szs, bool add = true ) {
		if ( add ) {
			for ( const auto& sz : szs ) {
				should_init.emplace_back( sz );
			}
			init_fonts = true;
		}

		if ( path && strlen( path ) > 0 ) {
			for ( const auto& sz : szs ) {
				fonts.insert( { sz, ImGui::GetIO( ).Fonts->AddFontFromFileTTF( path, sz, &config, combined_ranges.data( ) ) } );
			}
		} else if ( data && data_size > 0 ) {
			for ( const auto& sz : szs ) {
				fonts.insert( { sz, ImGui::GetIO( ).Fonts->AddFontFromMemoryTTF( data, data_size, sz, &config, combined_ranges.data( ) ) } );
			}
		}
	}

	ImFont* get( float sz ) {
		auto it = fonts.find( sz );

		if ( it == fonts.end( ) ) {
			if ( std::find( should_init.begin( ), should_init.end( ), sz ) == should_init.end( ) ) {
				should_init.emplace_back( sz );
				init_fonts = true;
			}

			return ImGui::GetIO( ).Fonts->Fonts[0];
		}

		return it->second;
	}

	void set_data( const char* _path ) {
        path = ( const char* )_path;
	}

	void set_data( unsigned char* bytes, size_t size ) {
		if ( data ) {
			delete[] data;
		}

        data = new unsigned char[size];
        std::memcpy( data, bytes, size );

        data_size = size;
	}

	void set_config( const ImFontConfig _config ) {
		config = ( ImFontConfig )_config;
	}

	void set_ranges( const ImWchar* _ranges ) {
		combined_ranges.clear( );

        for ( int i = 0; default_ranges[i] != 0; ++i ) {
            combined_ranges.push_back( default_ranges[i] );
        }

        if ( _ranges ) {
            for ( int i = 0; _ranges[i] != 0; ++i ) {
                combined_ranges.push_back( _ranges[i] );
            }
        }

        combined_ranges.push_back( 0 );
	}

	auto& get_fonts( ) {
		return fonts;
	}
};

enum fonts_ {
    font,
    size
};

inline std::vector< c_font > fonts( fonts_::size );