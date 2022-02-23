/*
 *  GXML/Graphite: Geometry and Graphics Programming Library + Utilities
 *  Copyright (C) 2000 Bruno Levy
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  If you modify this software, you should include a notice giving the
 *  name of the person performing the modification, the date of modification,
 *  and the reason for such modification.
 *
 *  Contact: Bruno Levy
 *
 *     levy@loria.fr
 *
 *     ISA Project
 *     LORIA, INRIA Lorraine, 
 *     Campus Scientifique, BP 239
 *     54506 VANDOEUVRE LES NANCY CEDEX 
 *     FRANCE
 *
 *  Note that the GNU General Public License does not permit incorporating
 *  the Software into proprietary programs. 
 */

#ifndef H_OGF_META_MEMBERS_META_METHOD_H
#define H_OGF_META_MEMBERS_META_METHOD_H

#include <OGF/gom/common/common.h>
#include <OGF/gom/reflection/meta_member.h>
#include <OGF/gom/reflection/meta_arg.h>

/**
 * \file OGF/gom/reflection/meta_method.h
 * \brief Meta-information attached to class methods.
 */

namespace OGF {

    class Object ;
    class ArgList ;

    /**
     * \brief Function pointer types for method adapters.
     * \details A GOM Method adapter is a function extracting the
     * arguments from an ArgList, calling a function, and
     * transforming the result into a return value (this
     * process is referred to as "Marshalling" in distributed
     * objects litterature). GOM Method adapters are automatically
     * generated by the GOM generator.
     */
    typedef bool (*gom_method_adapter)(
        Object* target, 
        const std::string& method_name, const ArgList& args,
        Any& ret_val
    ) ;

    /**
     * \brief The representation of a method in the Meta repository.
     */
    gom_class GOM_API MetaMethod : public MetaMember {
    public:

        /**
         * \brief MetaMethod constructor
         * \param[in] name name of the method
         * \param[in] container the MetaClass this method belongs to
         * \param[in] return_type C++ name of the return type
         */
        MetaMethod(
            const std::string& name,
            MetaClass* container,
            const std::string& return_type 
        ) ;

        /**
         * \brief MetaMethod constructor
         * \param[in] name name of the method
         * \param[in] container the MetaClass this method belongs to
         * \param[in] return_type pointer to the MetaType that corresponds 
         *  to the return type
         */
        MetaMethod(
            const std::string& name,
            MetaClass* container,
            MetaType* return_type
        ) ;

        /**
         * \brief MetaMethod destructor.
         */
	~MetaMethod() override;

	/**
	 * \copydoc MetaMember::pre_delete()
	 */
	void pre_delete() override;
	
    gom_slots:
        
        /**
         * \brief Gets the number of arguments.
         * \return the number of arguments of the method
         */
        size_t nb_args() const {
            return meta_args_.size();
        }

        /**
         * \brief Gets the name of an argument by index.
         * \param[in] i index of the argument
         * \return the name of the \p i th argument
         */
        const std::string& ith_arg_name(index_t i) const {
            ogf_assert(i < meta_args_.size());
            return meta_args_[i].name();
        }

        /**
         * \brief Gets the type name of an argument by index.
         * \param[in] i index of the argument
         * \return the type name of the \p i th argument
         */
        const std::string& ith_arg_type_name(index_t i) const {
            ogf_assert(i < meta_args_.size());
            return meta_args_[i].type_name();
        }

        /**
         * \brief Gets the type of an argument by index.
         * \param[in] i index of the argument
         * \return the type name of the \p i th argument
         */
	MetaType* ith_arg_type(index_t i) const {
            ogf_assert(i < meta_args_.size());
            return meta_args_[i].type();
	}

        /**
         * \brief Tests whether an argument has a default value.
         * \param[in] i index of the argument
         * \retval true if argument \p i has a default value
         * \retval false otherwise
         */
        bool ith_arg_has_default_value(index_t i) const {
            ogf_assert(i < meta_args_.size());
            return meta_args_[i].has_default_value();
        }

        /**
         * \brief Gets the default value of an argument
         * \param[in] i index of the argument
         * \return the default value of argument \p i
         */
	std::string ith_arg_default_value_as_string(index_t i) const {
	    return ith_arg_default_value(i).as_string();
	}
	
        /**
         * \brief Gets the return type name.
         * \return the C++ name of the return type
         */
        const std::string& return_type_name() const { 
            return return_type_name_ ; 
        }

        /**
         * \brief Gets the return type.
         * \return a pointer to the MetaType that corresponds
         *  to the return type
         */
        MetaType* return_type() const ; 

	/**
	 * \brief Tests whether an argument has a custom attribute.
	 * \param[in] i the index of the argument, in 0..nb_args()-1.
	 * \param[in] name the name of the custom attribute.
	 * \retval true if the ith arg has the custom attribute.
	 * \retval false otherwise.
	 * \pre i < nb_args()
	 */
	bool ith_arg_has_custom_attribute(
	    index_t i, const std::string& name
	) const;

	/**
	 * \brief Gets the value of a custom attribute of an argument.
	 * \param[in] i the index of the argument, in 0..nb_args()-1.
	 * \param[in] name the name of the custom attribute.
	 * \return the value of the custom attribute.
	 * \retval false otherwise.
	 * \pre i < nb_args() && has_custom_attribute(i,name)
	 */
	std::string ith_arg_custom_attribute_value(
	    index_t i, const std::string& name
	) const;

	/**
	 * \brief Gets the number of custom attribute of an argument.
	 * \param[in] i the index of the argument, in 0..nb_args()-1.
	 * \return the number of custom attributes of the argument.
	 */
	size_t ith_arg_nb_custom_attributes(index_t i);

	/**
	 * \brief Gets a custom attribute name of an argument by index.
	 * \param[in] i the index of the argument, in 0..nb_args()-1.
	 * \param[in] j the index of the custom attribute, 
	 *   in 0..ith_arg_nb_custom_attributes(i)-1
	 * \return the custom attribute
	 * \pre i < nb_args() && j < nb_custom_attributes(i)
	 */
	std::string ith_arg_jth_custom_attribute_name(index_t i, index_t j);

	/**
	 * \brief Gets a custom attribute value of an argument by index.
	 * \param[in] i the index of the argument, in 0..nb_args()-1.
	 * \param[in] j the index of the custom attribute, 
	 *   in 0..ith_arg_nb_custom_attributes(i)-1
	 * \return the custom attribute
	 * \pre i < nb_args() && j < nb_custom_attributes(i)
	 */
	std::string ith_arg_jth_custom_attribute_value(index_t i, index_t j);
	
    public:
	
        /**
         * \brief Gets the default value of an argument
         * \param[in] i index of the argument
         * \return the default value of argument \p i
         */
        const Any& ith_arg_default_value(index_t i) const {
            ogf_assert(i < meta_args_.size());
            return meta_args_[i].default_value();
        }

	
        /**
         * \brief Gets a const MetaArg by index
         * \param i index of the argument
         * \return a const pointer to the MetaArg
         * \pre i < nb_args()
         */
        const MetaArg* ith_arg(index_t i) const {
            ogf_assert(i < meta_args_.size()) ;
            return &(meta_args_[i]) ;
        }

        /**
         * \brief Gets a MetaArg by index
         * \param i index of the argument
         * \return a pointer to the MetaArg
         * \pre i < nb_args()
         */
        MetaArg* ith_arg(index_t i) {
            ogf_assert(i < meta_args_.size()) ;
            return &(meta_args_[i]) ;
        }

        /**
         * \brief Adds a new argument to the method
         * \param arg a const reference to the MetaArg,
         *  it is copied and stored in the MetaMethod
         */
        void add_arg(const MetaArg& arg) {
            meta_args_.push_back(arg) ;
        }

        /**
         * \brief Tests whether the method has an argument
         *  of a given name.
         * \param[in] meta_arg_name name to be tested
         * \retval true if the method has an argument named \p meta_arg_name
         * \retval false otherwise
         */
        bool has_arg(const std::string& meta_arg_name) ;

        /**
         * \brief Finds an argument by name.
         * \param[in] meta_arg_name name of the argument
         * \return a const pointer to the MetaArg 
         * \pre has_arg(meta_arg_name)
         */
        const MetaArg* find_arg(const std::string& meta_arg_name) const ;

        /**
         * \brief Finds an argument by name.
         * \param[in] meta_arg_name name of the argument
         * \return a pointer to the MetaArg 
         * \pre has_arg(meta_arg_name)
         */
        MetaArg* find_arg(const std::string& meta_arg_name) ;


        /**
         * \brief Gets the method adapter.
         * \return the method adapter if available, else nil
         * \see gom_method_adapter
         */
        gom_method_adapter method_adapter() const {
            return adapter_ ;
        }

        /**
         * \brief Sets the method adapter.
         * \param[in] adapter the method adapter
         * \see gom_method_adapter
         */
        void set_method_adapter(gom_method_adapter adapter) {
            adapter_ = adapter ;
        }

        /**
         * \brief Invokes this method on a target object. 
         * \details The default invokation mechanism uses the method 
         *  adapter (i.e. a function pointer).
         * \param[in] target a pointer to the target object. It needs
         *  to be of the same class this method belongs to
         * \param[in] args a const reference to the arguments list
         * \param[out] return_value the return value, as a string
         * \pre target->meta_class() == meta_class()
         */
        virtual bool invoke(
            Object* target, const ArgList& args, Any& return_value
        ) ;

        /**
         * \brief Implements the dynamic invocation API for the MetaMethod
         *  object.
         * \details Will be used in future versions, where the Meta system
         *  is completely visible from Graphite Embedded Language.
         * \param[in] method_name name of the method
         * \param[in] args a const reference to the arguments list
         * \param[out] ret_val the return value, represented as an Any
         */
        virtual bool invoke(
            const std::string& method_name,
            const ArgList& args, Any& ret_val
        ) {
            return Object::invoke_method(method_name, args, ret_val) ;
        }

        /**
         * \brief Checks whether the specified ArgList contains all 
         *  the required args.
         * \details The ArgList is compatible with this MetaMethod if
         *  it has a named argument for each named parameter that does
         *  not have a default value.
         * \param[in] args a const reference to the arguments list
         * \retval true if the argument list \p args is compatible with
         *  this MetaMethod
         * \retval false otherwise
         */
        virtual bool check_args(const ArgList& args) ;

        /**
         * \brief Counts the number of arguments this method would use
         * when invoked on the specified args. 
         * \details The result is unspecified if check_args 
         *  returns false.
         * \param[in] args a const reference to the arguments list
         * \return the number of arguments that correspond to arguments
         *  of this MetaMethod
         */
        virtual index_t nb_used_args(const ArgList& args) ;

        /**
         * \brief Counts the number of arguments assigned with their default
         * value when this method is invoked on the specified args.
         * \details The result is unspecified if check_args returns false.
         * \param[in] args a const reference to the arguments list
         * \return the number of arguments assigned with their default 
         *  value when this method is invoked with \p args
         */
        virtual index_t nb_default_args(const ArgList& args) ;

        /**
         * \brief Adds the arguments with default values to an ArgList.
         * \details Each time an argument does not exist in ArgList \p args and
         *  has a default value, it is created in \p args with the default
         *  value.
         * \param[in,out] args the ArgList
         */
        virtual void add_default_args(ArgList& args) ;

    protected:
        /**
         * \brief Emits a signal in a target object.
         * \param[in] target a pointer to the target object
         * \param[in] sig_name name of the signal
         * \param[in] args a const reference to the argument list
         * \param[in] called_from_slot distingished between signals
         *  generated from the GUI and signals called programatically.
         */
        static bool emit_signal(
            Object* target, const std::string& sig_name, 
            const ArgList& args, bool called_from_slot = true
        ) ;

        /**
         * \brief Emits a signal from this MetaMethod.
         * \details Implements the Dynamic Invocation API for 
         *  MetaMethod, this will be used in future versions,
         *  that expose the entire Meta system to Graphite Embedded 
         *  Language.
         */
        bool emit_signal(
            const std::string& signal_name, 
            const ArgList& args, bool called_from_slot = true
        ) override {
           return emit_signal(this, signal_name, args, called_from_slot);
        }

    private:
        std::string return_type_name_ ;
        MetaArgList meta_args_ ;
        gom_method_adapter adapter_ ;
    } ;

    /**
     * \brief Automatic reference-counted pointer to a MetaMethod.
     */
    typedef SmartPointer<MetaMethod> MetaMethod_var ;

}
#endif 

