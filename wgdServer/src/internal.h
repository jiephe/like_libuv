#pragma once

#include <memory>
#include <map>
#include <functional>
#include <string>
#include <vector>
#include <thread>

namespace three_year
{
	class CWGDConn;
	class CBaseSocket;
	class CWgdServer;
	class CSimpleBuffer;

	using connPtr = std::shared_ptr<CWGDConn>;
	using socketPtr = std::shared_ptr<CBaseSocket>;
	using wgdSvrPtr = std::shared_ptr<CWgdServer>;
	using bufferPtr = std::shared_ptr<CSimpleBuffer>;

#define META_FUNCTION_H(type, name)			\
public:										\
	type get_##name() const;				\
	void set_##name(const type& name);		\
	void set_##name(type&& name);			\
private:									\
	type name##_;							

#define META_FUNCTION_CPP(type, className, name)		\
	type className::get_##name() const					\
	{													\
		return name##_;									\
	}													\
														\
	void className::set_##name(const type& name)		\
	{													\
		name##_ = name;									\
	}													\
														\
	void className::set_##name(type&& name)				\
	{													\
		name##_ = std::move(name);						\
	}		
}


