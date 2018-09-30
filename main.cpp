#include <array>
#include <functional>
#include <iostream>

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
		struct TypeList
		{
			static constexpr const unsigned int Size = 3; // TODO Make 
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

	template <typename Base, typename Self>
	struct Extend : public Base
	{
		template <typename ...T>
		Extend(T&&... args) :
			Base(std::forward<T>(args)...) // TODO Validate constructible
		{
			Base::set_type_index(utils::get_subtype_index<Base, Self>());
		}
	};

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

		/**
		 * 
		 * TODO
		 * 	Handle const overload
		 * 	Handle multiple methods
		 * 	Allow some params to not be variadic
		 */
		template <typename Self, typename _ReturnType, typename _Arg>
		class Base
		{
		public:
			using ReturnType = _ReturnType;
			using Arg = _Arg;

			ReturnType dispatch(typename Arg::Key param) {
				utils::TypeIndex index = param.get_type_index() - 1; // TODO Handle case without this method
				return _callbacks[index](std::forward<typename Arg::Key>(param));
			}

			Base() :
				_callbacks{create_callbacks<typename Arg::Key>(typename Arg::List{})}
			{

			}
		private:
			using Call = std::function<ReturnType (typename Arg::Key)>; // TODO Self* should be the first argument. This would allow copy and move easily
			std::array<Call, Arg::List::Size> _callbacks;

			template <typename Common, typename ...T>
			std::array<Call, Arg::List::Size> create_callbacks(const utils::TypeList<T...>&)
			{
				return {create_callback<Common, T>()...};
			}

			template <typename Common, typename T>
			Call create_callback()
			{
				return [&](Common param) -> ReturnType {
					return static_cast<Self*>(this)->handle(reinterpret_cast<const T&>(param)); // TODO Keep ptr/ref and const from common
				};
			}
		};
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
{};
struct Rectangle : laft::Extend<Form, Rectangle> // TODO How to ensure the same type is not called twice?
{};
struct Triangle : laft::Extend<Form, Triangle>
{};

using Arg1 = laft::multimethod::param::StaticList<const Form&, Circle, Rectangle, Triangle>;
struct Printer : laft::multimethod::Base<Printer, std::string, Arg1>
{
	using Base<Printer, std::string, Arg1>::Base;

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

	std::cout << "Result: " << Printer{}.dispatch(Circle{"test1"}) << std::endl;
	std::cout << "Result: " << Printer{}.dispatch(Rectangle{"test2"}) << std::endl;
	std::cout << "Result: " << Printer{}.dispatch(Triangle{"test3"}) << std::endl;

	return 0;
}