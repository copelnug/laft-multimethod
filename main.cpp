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

			template <typename U>
			using Append = TypeList<T...,U>;
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

	/**
		@brief 

		We use Impl::ExtendTypeRegister to basically append the registering to any
		base constructor. As we inherit from it after Base, the constructor of
		ExtenderTypeRegister will be executed after ANY constructor from base. Thus
		allowing to inherit base constructors AND always register the type.
	*/
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
			template <typename Common, typename T>
			struct TypeCast
			{
				using Arg = Common;

				TypeCast(Common param) : param_{std::forward<Common>(param)} {}

				const T& cast() { return reinterpret_cast<const T&>(std::forward<Common>(param_)); }
				Common param_;
			};

			template <typename Function, typename ...Receiving, typename ...T>
			auto createCallback(Function*, utils::TypeList<Receiving...>*, utils::TypeList<T...>* t)
				-> std::function< typename Function::ReturnType(const Function&, Receiving...) >
			{
				// We create an inside lambda that take T and then call it with the parameters and use default constructor of T for conversion.
				return [](const Function& function, Receiving&&... params) -> typename Function::ReturnType {
					return [](const Function& function, T&&... ts) -> typename Function::ReturnType {
						return function.handle(ts.cast()...); // TODO Keep ptr/ref and const from common
					}(function, params...);
				};
			}
			
			template <typename Function, typename ...Receiving, typename ...Processed>
			auto createHelper(Function* f, utils::TypeList<Receiving...>* receiving, utils::TypeList<Processed...>* processed, utils::TypeList<>*)
			{
				return createCallback(f, receiving, processed);
			}
			template <typename Function, typename ...Receiving, typename ...Processed, typename Common, typename ...T, typename ...U>
			auto createHelper(Function* f, utils::TypeList<Receiving...>*, utils::TypeList<Processed...>*, utils::TypeList<param::StaticList<const Common&, T...>, U...>* u) // TODO Better overload support?
			{
				return std::array{
					createHelper(
						f,
						static_cast<utils::TypeList<Receiving..., Common>*>(nullptr),
						static_cast<utils::TypeList<Processed..., TypeCast<const Common&,T>>*>(nullptr),
						static_cast<utils::TypeList<U...>*>(nullptr)
					)...
				};
			}
			/*template <typename Function, typename ...Processed, typename T, typename ...U>
			auto createHelper(Function* f, utils::TypeList<Processed...>*, T*, U*... u)
			{
				return createHelper(
					f,
					static_cast<utils::TypeList<Processed..., TypeCast<T,T>>*>(nullptr),
					u...
				);
			}*/

			template <typename Function, typename ...T>
			auto createHelperWrapper(Function* f, laft::utils::TypeList<T...>* t)
			{
				return createHelper(
					f,
					static_cast<utils::TypeList<>*>(nullptr),
					static_cast<utils::TypeList<>*>(nullptr),
					t
				);
			}

			template <typename Function>
			auto createArray()
			{
				// Store in static variable to only build once per Function type.
				static const auto array = createHelperWrapper(static_cast<Function*>(nullptr), static_cast<typename Function::ArgList*>(nullptr));
				return array;
			}

			/*template <typename Array, typename T>
			auto getArray(const Array& array, T&&)
			{
				return array;
			}*/
			template <typename T, long unsigned int N>
			auto getArray(const std::array<T,N>& array, const Identifiable& identifiable)
			{
				return array[identifiable.get_type_index()-1];// TODO security
			}

			/*template <typename Array, typename ...T>
			auto callHelper(const Array& array, T&&... params);*/
			template <typename Array>
			auto callHelper(const Array& array)
			{
				return array;
			}
			template <typename Array, typename T, typename ...U>
			auto callHelper(const Array& array, T&& p, U&&... others)
			{
				return callHelper(
					getArray(array, p),
					others...
				);
			}

			template <typename Array, typename Function, typename ...T>
			auto call(const Array& array, const Function& f, T&&... params) -> typename Function::ReturnType
			{
				auto func = callHelper(array, params...);
				return func(f, params...);
			}
		}
		/**
		 * 
		 * 
		 * Function should have:
		 * 	ReturnType
		 * 	ArgList
		 */
		template <typename Function, typename ...T>
		auto dispatch(const Function& function, T&&... params) -> typename Function::ReturnType
		{
			const auto& array = impl::createArray<Function>();
			return impl::call(array, function, params...);
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

using AnyForm = laft::multimethod::param::StaticList<const Form&, Circle, Rectangle, Triangle>; // TODO Any way to match the index in there with the get_subtype_index no matter the order? Should be possible to build an array. We may need get_max_index for the size.
struct Printer
{
	using ReturnType = std::string;
	using Call = std::function<std::string(const Printer&, const Form&)>;
	using Arg = AnyForm;
	using ArgList = laft::utils::TypeList<
		AnyForm
	>;

	std::string handle(const Circle& circle) const
	{
		return "circle";
	}
	std::string handle(const Rectangle& circle) const
	{
		return "rectangle";
	}
	std::string handle(const Triangle& circle) const
	{
		return "triangle";
	}
};
struct Intersect
{
	using ReturnType = std::string;
	using Call = std::function<std::string(const Intersect&, const Form&, const Form&)>;
	using ArgList = laft::utils::TypeList<
		AnyForm,
		AnyForm
	>;

	Intersect(Intersect&) = delete;

	std::string handle(const Circle&, const Circle&) const
	{
		return "Intersect two circles";
	}
	std::string handle(const Circle&, const Rectangle&) const
	{
		return "Intersect circle & rectangle";
	}
	std::string handle(const Circle&, const Triangle&) const
	{
		return "Intersect circle & triangle";
	}
	std::string handle(const Rectangle&, const Circle&) const
	{
		return "Intersect rectangle & circle";
	}
	std::string handle(const Rectangle&, const Rectangle&) const
	{
		return "Intersect rectangle & rectangle";
	}
	std::string handle(const Rectangle&, const Triangle&) const
	{
		return "Intersect rectangle & triangle";
	}
	std::string handle(const Triangle&, const Circle&) const
	{
		return "Intersect triangle & circle";
	}
	std::string handle(const Triangle&, const Rectangle&) const
	{
		return "Intersect triangle & rectangle";
	}
	std::string handle(const Triangle&, const Triangle&) const
	{
		return "Intersect triangle & triangle";
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

	std::cout << "Result: " << dispatch(Intersect{}, Circle{"Test 1"}, Rectangle{"Test 2"}) << std::endl;

	// TODO
	//	handle non const reference
	//	hanled moving in parameters. (rvalue)
	return 0;
}
