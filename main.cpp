#include <array>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <sys/types.h>

namespace laft
{
	namespace utils
	{
		using TypeIndex = unsigned int;

		template <typename Key>
		TypeIndex get_next_type_index()
		{
			static TypeIndex current{0};
			++current;
			return current;
		}

		template <typename Base, typename Subtype>
		TypeIndex get_subtype_index()
		{
			static TypeIndex index{get_next_type_index<Base>()};
			return index;
		}

		template <typename ...T>
		struct TypeCount;

		template <typename T, typename ...U>
		struct TypeCount<T, U...>
		{
			constexpr const static unsigned int Size = TypeCount<U...>::Size + 1;
		};
		template <>
		struct TypeCount<>
		{
			constexpr const static unsigned int Size = 0;
		};

		template <typename ...T>
		struct TypeList
		{
			static constexpr const unsigned int Size = TypeCount<T...>::Size;
		};
	}

	class Identifiable // TODO Better name
	{
	public:

		utils::TypeIndex get_type_index() const
		{
			return _typeIndex;
		}
		// TODO Make private/friend? Also, we need to be sure it is always called. Maybe the constructor could require an opaque type.
		void set_type_index(utils::TypeIndex typeIndex)
		{
			_typeIndex = typeIndex;
		}
	private:
		utils::TypeIndex _typeIndex{0};
	};

	namespace Impl
	{
		/*
			\brief This class is simply a way to append operations to all constructors.

			Extending it simply allow to register the type in all constructors.
		*/
		template <typename Base, typename Self>
		struct ExtendTypeRegister
		{
			ExtendTypeRegister();
		};
	}

	template <typename Base, typename Self>
	struct Extend : public Base, private Impl::ExtendTypeRegister<Base,Self>
	{
		using Base::Base;

		friend Impl::ExtendTypeRegister<Base,Self>;
	};
	template <typename Base, typename Self>
	Impl::ExtendTypeRegister<Base,Self>::ExtendTypeRegister()
	{
		static_cast<Extend<Base,Self>&>(*this).Base::set_type_index(utils::get_subtype_index<Base, Self>());
	}

	namespace multimethod
	{
		namespace param
		{
			template <typename Common, typename ...T>
			struct StaticList
			{
				using Key = Common;
				using List = utils::TypeList<T...>;
			};
		}

		namespace impl
		{
			template <typename Function, typename Common, typename T>
			auto createCallback() -> typename Function::Call
			{
				return [](Function function, Common param) -> typename Function::ReturnType {
					return function.handle(reinterpret_cast<const T&>(param)); // TODO Keep ptr/ref and const from common
				};
			}

			template <typename Function, typename Common, typename ...T>
			std::array<typename Function::Call, Function::Arg::List::Size> createArray(utils::TypeList<T...>*)
			{
				return {createCallback<Function, Common, T>()...};
			}

			template <typename Function, typename Arg>
			auto createArray() -> std::array<typename Function::Call, Function::Arg::List::Size>
			{
				return createArray<Function, typename Arg::Key>(static_cast<typename Arg::List*>(nullptr));
			}

			/**
			 * Allow to get the function array to use for a specified functor.
			 * \tparam Function Functor object.
			 * \return Array of function.
			 * 
			 * This method primary use is to create the array only once. Then it can be obtained from the static variable
			 * each time it is needed.
			 */
			template <typename Function>
			auto createArray(const Function&) -> std::array<typename Function::Call, Function::Arg::List::Size>
			{
				static const auto array = createArray<Function, typename Function::Arg>();
				return array;
			}
		}
		/**
		 * 
		 * 
		 * Function should have:
		 * 	ReturnType
		 * 	Call : std::function<ReturnType(Function, Function::Arg::Key)
		 * 	Arg : param::StaticList
		 */
		template <typename Function, typename T>
		auto dispatch(Function function, T&& param) -> typename Function::ReturnType
		{
			const auto& array = impl::createArray(function);
			utils::TypeIndex index = param.get_type_index() - 1; // TODO Handle case without this method
			// TODO Throw exception if get_type_index() returns 0. It means the object is not done initializing.
			return array[index](function, std::forward<typename Function::Arg::Key>(param));
		}
	}
}

struct Form : laft::Identifiable
{
	Form(std::string name) :
		_name(std::move(name))
	{}

private:
	std::string _name;
};

struct Circle;
struct Rectangle;
struct Triangle;


struct Circle : laft::Extend<Form, Circle>
{
	using Extend<Form,Circle>::Extend;
};
struct Rectangle : laft::Extend<Form, Rectangle> // TODO How to ensure the same type is not called twice?
{
	using Extend<Form,Rectangle>::Extend;
};
struct Triangle : laft::Extend<Form, Triangle>
{
	using Extend<Form,Triangle>::Extend;
};

using Arg1 = laft::multimethod::param::StaticList<const Form&, Circle, Rectangle, Triangle>;
struct Printer
{
	using ReturnType = std::string;
	using Call = std::function<std::string(const Printer&, const Form&)>;
	using Arg = Arg1;

	std::string handle(const Circle& circle)
	{
		return "circle";
	}
	std::string handle(const Rectangle& circle)
	{
		return "rectangle";
	}
	std::string handle(const Triangle& circle)
	{
		return "triangle";
	}
};

int main()
{
	std::cout << "(int, int) => " << laft::utils::get_subtype_index<int, int>() << std::endl;
	std::cout << "(int, short) => " << laft::utils::get_subtype_index<int, short>() << std::endl;
	std::cout << "(int, char) => " << laft::utils::get_subtype_index<int, char>() << std::endl;
	std::cout << "(char, char) => " << laft::utils::get_subtype_index<char, char>() << std::endl;
	std::cout << "(char, short) => " << laft::utils::get_subtype_index<char, short>() << std::endl;
	std::cout << "(char, int) => " << laft::utils::get_subtype_index<char, int>() << std::endl;
	std::cout << "(char, int) => " << laft::utils::get_subtype_index<char, int>() << std::endl;

	std::cout << "Circle => " << laft::utils::get_subtype_index<Form, Circle>() << std::endl;
	std::cout << "Rectangle => " << laft::utils::get_subtype_index<Form, Rectangle>() << std::endl;
	std::cout << "Triangle => " << laft::utils::get_subtype_index<Form, Triangle>() << std::endl;

	using laft::multimethod::dispatch;
	std::cout << "Result: " << dispatch(Printer{}, Circle{"test1"}) << std::endl;
	std::cout << "Result: " << dispatch(Printer{}, Rectangle{"test2"}) << std::endl;
	std::cout << "Result: " << dispatch(Printer{}, Triangle{"test3"}) << std::endl;

	return 0;
}
