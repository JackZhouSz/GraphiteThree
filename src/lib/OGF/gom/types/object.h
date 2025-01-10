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

#ifndef H_OGF_GOM_TYPES_OBJECT_H
#define H_OGF_GOM_TYPES_OBJECT_H

#include <OGF/gom/common/common.h>
#include <OGF/gom/types/gom_defs.h>
#include <OGF/gom/types/arg_list.h>

#include <string>
#include <vector>

/**
 * \file OGF/gom/types/object.h
 * \brief The base class for all objects in the GOM system.
 */

namespace OGF {

    class MetaType;
    class MetaClass;
    class Object;
    class Connection;
    class ConnectionTable;
    class Interpreter;

    /**
     * \brief Base class for all objects in the GOM system.
     */
    gom_attribute(abstract,"true")
    gom_class GOM_API Object : public Counted {
    public:
        /**
         * \brief Object constructor.
	 * \param[in] transient if true, the object is transient.
	 *  Transient objects have their id set to zero and are
	 *  not referenced in the global id_to_object_ table.
         */
        explicit Object(bool transient = false);

        /**
         * \brief Object destructor.
         */
        ~Object() override;

        // Run-time type information

        /**
         * \brief Gets the meta class.
         * \return a pointer to the MetaClass of this object.
         */
        virtual MetaClass* meta_class() const;

        /**
         * \brief Sets the meta class.
         * \details This function is automatically called
         *  by the factories generated by GOMGEN. User code
         *  does not need to use it.
         * \param[in] mclass a pointer to the MetaClass
         */
        virtual void set_meta_class(MetaClass* mclass);

        /**
         * \brief Gets the identifier of this object.
         * \details Each object instance has a unique identifier.
         *  This identifier is used by the object maps managed
         *  by each MetaClass. It is also used by the VCR mechanism
         *  that allows one to record and playback all the events
         *  in the system.
         * \return the unique identifier of this object
         * \see string_id()
         */
        unsigned int id() const {
            return id_;
        }

        /**
         * \brief Gets the unique string identifier.
         * \details The unique string identifier is composed of
         *  the class name and instance id.
         * \return the unique string identifier
         * \see id()
         */
        std::string string_id() const;

        // Properties and Dynamic invokation interface

	/**
	 * \brief Tests whether a method is defined
         * \param[in] method_name name of the property
         * \retval true if this Object has the method
         * \retval false otherwise
         */
	bool has_method(const std::string& method_name) const;

        /**
         * \brief Invokes a method by method name and argument list,
         *  and gets the return value.
         * \param[in] method_name name of the method
         * \param[in] args a const reference to the ArgList
         * \param[out] ret_val the return value as an Any
         * \retval true if the method could be sucessfully invoked
         * \retval false otherwise
         */
        virtual bool invoke_method(
            const std::string& method_name,
            const ArgList& args, Any& ret_val
        );

        /**
         * \brief Invokes a method by method name and argument list.
         * \details This variant of invoke() is for methods with void
         *   return type.
         * \param[in] method_name name of the method
         * \param[in] args a const reference to the ArgList
         * \retval true if the method could be sucessfully invoked
         * \retval false otherwise
         */
        bool invoke_method(
	    const std::string& method_name, const ArgList& args
	) {
            Any ret_val;
            return invoke_method(method_name, args, ret_val);
        }

        /**
         * \brief Invokes a method by method name.
         * \details This variant of invoke() is for methods with no
         *  argument and void return type.
         * \param[in] method_name name of the method
         * \retval true if the method could be sucessfully invoked
         * \retval false otherwise
         */
        bool invoke_method(const std::string& method_name) {
            ArgList args;
            Any ret_val;
            return invoke_method(method_name, args, ret_val);
        }

	/**
	 * \brief Tests whether a property is defined
         * \param[in] prop_name name of the property
         * \retval true if this Object has the property
         * \retval false otherwise
         */
	bool has_property(const std::string& prop_name) const;

        /**
         * \brief Gets a property
         * \param[in] prop_name name of the property
         * \param[out] prop_value value of the property as a string
         * \retval true if the property could be sucessfully read
         * \retval false otherwise
         */
        virtual bool get_property(
            const std::string& prop_name, std::string& prop_value
        ) const;

        // ************** Signals and slots *************************

        /**
         * \brief Connects a signal with a slot of another object.
         * \param[in] signal_name name of the signal
         * \param[in] to a pointer to the receiver Object
         * \param[in] slot_name name of the receiver's slot
         * \return a pointer to the newly created Connection
         */
        virtual Connection* connect_signal_to_slot(
            const std::string& signal_name,
            Object* to, const std::string& slot_name
        );

        /**
         * \brief Adds a connection to this object
         * \param[in] connection a pointer to the connection to be added
         */
        virtual void add_connection(Connection* connection);

        /**
         * \brief Removes a connection to this object
         * \param[in] connection a pointer to the connection to be removed
         */
        virtual void remove_connection(Connection* connection);

	/**
	 * \brief Gets an object from a unique object id.
	 * \param[in] id the object id.
	 * \return a pointer to the object or nullptr if there
	 *  is no such object.
	 * \see id().
	 */
	static Object* id_to_object(unsigned int id) {
	    if(id_to_object_ == nullptr) {
		return nullptr;
	    }
	    auto it = id_to_object_->find(id);
	    if(it == id_to_object_->end()) {
		return nullptr;
	    }
	    return it->second;
	}

	/**
	 * \brief Gets an element by index.
	 * \details Part of the array interface, used by operator[]
	 *  in scripting language.
	 * \param[in] i the index of the element, in [0..get_nb_elements()-1].
	 * \param[out] value the element at index i.
	 */
	virtual void get_element(index_t i, Any& value) const;

	/**
	 * \brief Sets an element by index.
	 * \details Part of the array interface, used by operator[]
	 *  in scripting language.
	 * \param[in] i the index of the element, in [0..get_nb_elements()-1].
	 * \param[in] value the element at index i.
	 */
	virtual void set_element(index_t i, const Any& value);

	/**
	 * \brief Gets an element by item and component.
	 * \param[in] item in 0..nb_items()-1
	 * \param[in] component in 0..dimension()-1
	 * \param[out] value the value of the element, stored in an Any.
	 */
	void get_element(
	    index_t item, index_t component, Any& value
	) const {
	    get_element(item * get_dimension() + component, value);
	}

	/**
	 * \brief Sets an element by item and component.
	 * \param[in] item in 0..nb_items()-1
	 * \param[in] component in 0..dimension()-1
	 * \param[in] value the value of the element, stored in an Any.
	 */
	void set_element(
	    index_t item, index_t component, const Any& value
	) {
	    set_element(item * get_dimension() + component, value);
	}

        /**
         * \brief Displays the names of all objects that
         *   contain a substring
         * \param[in] needle the substring
         * \param[in] path the path to be prepended to the names
         */
        virtual void search(
            const std::string& needle, const std::string& path=""
        );

    gom_properties:

	/**
	 * \brief Gets the number of elements.
	 * \details Part of the array interface, used by operator[]
	 *  in scripting language.
	 */
	virtual index_t get_nb_elements() const;


	/**
	 * \brief Gets the number of elements per item.
	 * \details Part of the array interface, used by operator[]
	 *  in scripting language.
	 */
	virtual index_t get_dimension() const;

        /**
         * \brief Tests wheter signals are enabled.
         * \retval true if signals are enabled
         * \retval false otherwise
         */
        bool get_signals_enabled() const {
            return signals_enabled_;
        }

        /**
         * \brief Enables or disables signals.
         * \param[in] value true if signals should be enabled,
         *  false if they should be disabled.
         */
        void set_signals_enabled(bool value) {
            signals_enabled_ = value;
        }

        /**
         * \brief Tests wheter slots are enabled.
         * \retval true if slots are enabled
         * \retval false otherwise
         */
        bool get_slots_enabled() const {
            return slots_enabled_;
        }

        /**
         * \brief Enables or disables slots.
         * \param[in] value true if slots should be enabled,
         *  false if they should be disabled.
         */
        void set_slots_enabled(bool value) {
            slots_enabled_ = value;
        }

        /**
         * \brief Gets the meta class.
         * \return a pointer to the MetaClass of this object.
         */
        MetaClass* get_meta_class() const {
            return meta_class();
        }

        /**
         * \brief Gets the unique string identifier.
         * \details The unique string identifier is composed of
         *  the class name and instance id.
         * \return the unique string identifier
         * \see string_id(), id()
         */
	std::string get_string_id() const {
	    return string_id();
	}

        /**
         * \brief Gets the documentation.
         * \return A string with a human-readable documentation
         *  about this object.
         */
        virtual std::string get_doc() const;


    gom_slots:

        /**
         * \brief Tests whether two objects are equal
         * \details Default implementation just tests for pointers equality,
         *   derived classes may overload this function.
         * \param[in] other the other object to test equality with
         * \retval true if the two objects are equal
         * \retval false otherwise
         */
        bool equals(const Object* other) const;

        /**
         * \brief Compares this object with another one
         * \details Default implementation compares addresses
         * \param[in] other the other object to be compared
         * \retval POSITIVE if this object is greater than \p other
         * \retval ZERO if this object and \p other are equal
         * \retval NEGATIVE if this object is lower than \p other
         */
        virtual Sign compare(const Object* other) const;



        /**
         * \brief Tests whether this object inherits a given type.
         * \param[in] type a const pointer to the MetaType to be tested.
         * \retval true if this object inherits \p type
         * \retval false otherwise
         */
        virtual bool is_a(const MetaType* type) const;

	/**
	 * \brief Removes all connections from signals of
	 *  this objects.
	 * \details Connections to slots of this object are
	 *  kept.
	 */
	void disconnect();

        /**
         * \brief Enables signals.
         */
        void enable_signals() {
            signals_enabled_ = true;
        }

        /**
         * \brief Disables signals.
         */
        void disable_signals() {
            signals_enabled_ = false;
        }

        /**
         * \brief Enables slots.
         */
        void enable_slots() {
            slots_enabled_ = true;
        }

        /**
         * \brief Disables slots.
         */
        void disable_slots() {
            slots_enabled_ = false;
        }

        /**
         * \brief Sets several properties in a single call.
         * \param[in] args a const reference to a set of name-value pairs
         */
        void set_properties(const ArgList& args);

        /**
         * \brief Sets an individual property
         * \param[in] name name of the property
         * \param[in] value value of the property
         * \retval true if the property could be sucessfully set
         * \retval false otherwise
         */
        virtual bool set_property(
            const std::string& name, const std::string& value
        );

        /**
         * \brief Displays some help about this object
         * \details Outputs the doc property to the logger.
         */
        void help() const;

      public:
	// Note: set_property() variants that are not slots
	// need to be declared *after* the set_property() slot
	// else gomgen does not see the set_property slot.

        /**
         * \brief Sets an individual property
         * \param[in] name name of the property
         * \param[in] value value of the property as an Any
         * \retval true if the property could be sucessfully set
         * \retval false otherwise
         */
        virtual bool set_property(
            const std::string& name, const Any& value
        );

        /**
         * \brief Gets a property
         * \param[in] prop_name name of the property
         * \param[out] prop_value value of the property as a string
         * \retval true if the property could be sucessfully read
         * \retval false otherwise
         */
        virtual bool get_property(
            const std::string& prop_name, Any& prop_value
        ) const;

    protected:

        /**
         * \brief Emits a signal and calls the slots it is connected to.
         * \details This function is used by GOMGEN to implement all the
         *  signal adapters, that marshall the signal's arguments in
         *  the argument list and calls the slots.
         * \param[in] signal_name name of the signal
         * \param[in] args a const reference to the arguments list
         * \param [in] called_from_slot distingishes whether
         * the signal was called after an event, or was called
         * from client code. This can be used to implement a
         * recording mechanism.
         */
        virtual bool emit_signal(
            const std::string& signal_name, const ArgList& args,
            bool called_from_slot = false
        );

    private:
        MetaClass* meta_class_;
        ConnectionTable* connections_;
        bool signals_enabled_;
        bool slots_enabled_;
        unsigned int id_;

        // Signal method adapter needs to call
        // emit_signal() which is protected.
        friend class MetaMethod;
	static std::map<index_t, Object*>* id_to_object_;
    };

    /**************************************************************/

    /**
     * \brief An automatic reference-counted pointer to an Object.
     */
    typedef SmartPointer<Object> Object_var;

    /**************************************************************/

}

#endif
