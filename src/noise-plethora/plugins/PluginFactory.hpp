
#pragma once

#include <memory>
#include <string> // string might not be allowed
#include <functional>
#include <map>

#include "NoisePlethoraPlugin.hpp"

class NoisePlethoraPlugin;


class MyFactory {
public:
	static MyFactory* Instance() {
		static MyFactory factory;
		return &factory;
	}

	std::shared_ptr<NoisePlethoraPlugin> Create(std::string name) {
		NoisePlethoraPlugin* instance = nullptr;

		// find name in the registry and call factory method.
		auto it = factoryFunctionRegistry.find(name);
		if (it != factoryFunctionRegistry.end()) {
			instance = it->second();
		}

		// wrap instance in a shared ptr and return
		if (instance != nullptr) {
			return std::shared_ptr<NoisePlethoraPlugin>(instance);
		}
		else {
			return nullptr;
		}
	}

	void RegisterFactoryFunction(std::string name, std::function<NoisePlethoraPlugin*(void)> classFactoryFunction) {
		// register the class factory function
		factoryFunctionRegistry[name] = classFactoryFunction;
	}

	std::map<std::string, std::function<NoisePlethoraPlugin*(void)>> factoryFunctionRegistry;
};

template<class T> class Registrar {
public:
	Registrar(std::string className) {
		// register the class factory function
		MyFactory::Instance()->RegisterFactoryFunction(className,
		    [](void) -> NoisePlethoraPlugin * { return new T();});
	}
};


