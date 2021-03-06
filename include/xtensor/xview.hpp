/***************************************************************************
* Copyright (c) 2016, Johan Mabille, Sylvain Corlay and Wolf Vollprecht    *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XTENSOR_VIEW_HPP
#define XTENSOR_VIEW_HPP

#include <algorithm>
#include <array>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

#include <xtl/xclosure.hpp>
#include <xtl/xsequence.hpp>

#include "xbroadcast.hpp"
#include "xcontainer.hpp"
#include "xiterable.hpp"
#include "xsemantic.hpp"
#include "xtensor_forward.hpp"
#include "xview_utils.hpp"

namespace xt
{

    /*********************
     * xview declaration *
     *********************/

    template <class CT, class... S>
    struct xcontainer_inner_types<xview<CT, S...>>
    {
        using xexpression_type = std::decay_t<CT>;
        using temporary_type = view_temporary_type_t<xexpression_type, S...>;
    };

    template <bool is_const, class CT, class... S>
    class xview_stepper;

    template <class ST, class... S>
    struct xview_shape_type;

    template <class CT, class... S>
    struct xiterable_inner_types<xview<CT, S...>>
    {
        using xexpression_type = std::decay_t<CT>;
        using inner_shape_type = typename xview_shape_type<typename xexpression_type::shape_type, S...>::type;
        using stepper = xview_stepper<std::is_const<std::remove_reference_t<CT>>::value, CT, S...>;
        using const_stepper = xview_stepper<true, std::remove_cv_t<CT>, S...>;
    };

    /**
     * @class xview
     * @brief Multidimensional view with tensor semantic.
     *
     * The xview class implements a multidimensional view with tensor
     * semantic. It is used to adapt the shape of an xexpression without
     * changing it. xview is not meant to be used directly, but
     * only with the \ref view helper functions.
     *
     * @tparam CT the closure type of the \ref xexpression to adapt
     * @tparam S the slices type describing the shape adaptation
     *
     * @sa view, range, all, newaxis
     */
    template <class CT, class... S>
    class xview : public xview_semantic<xview<CT, S...>>,
                  public xiterable<xview<CT, S...>>
    {
    public:

        using self_type = xview<CT, S...>;
        using xexpression_type = std::decay_t<CT>;
        using semantic_base = xview_semantic<self_type>;

        static constexpr bool is_const = std::is_const<std::remove_reference_t<CT>>::value;
        using value_type = typename xexpression_type::value_type;
        using reference = std::conditional_t<is_const,
                                             typename xexpression_type::const_reference,
                                             typename xexpression_type::reference>;
        using const_reference = typename xexpression_type::const_reference;
        using pointer = std::conditional_t<is_const,
                                           typename xexpression_type::const_pointer,
                                           typename xexpression_type::pointer>;
        using const_pointer = typename xexpression_type::const_pointer;
        using size_type = typename xexpression_type::size_type;
        using difference_type = typename xexpression_type::difference_type;

        using iterable_base = xiterable<self_type>;
        using inner_shape_type = typename iterable_base::inner_shape_type;
        using shape_type = inner_shape_type;
        using strides_type = shape_type;

        using slice_type = std::tuple<S...>;

        using stepper = typename iterable_base::stepper;
        using const_stepper = typename iterable_base::const_stepper;

        static constexpr layout_type static_layout = layout_type::dynamic;
        static constexpr bool contiguous_layout = false;

        // The FSL argument prevents the compiler from calling this constructor
        // instead of the copy constructor when sizeof...(SL) == 0.
        template <class CTA, class FSL, class... SL>
        explicit xview(CTA&& e, FSL&& first_slice, SL&&... slices) noexcept;

        xview(const xview&) = default;
        self_type& operator=(const xview& rhs);

        template <class E>
        self_type& operator=(const xexpression<E>& e);

        template <class E>
        disable_xexpression<E, self_type>& operator=(const E& e);

        size_type dimension() const noexcept;

        size_type size() const noexcept;
        const inner_shape_type& shape() const noexcept;
        const slice_type& slices() const noexcept;
        layout_type layout() const noexcept;

        template <class... Args>
        reference operator()(Args... args);
        template <class... Args>
        reference at(Args... args);
        template <class OS>
        disable_integral_t<OS, reference> operator[](const OS& index);
        template <class I>
        reference operator[](std::initializer_list<I> index);
        reference operator[](size_type i);
        template <class It>
        reference element(It first, It last);

        template <class... Args>
        const_reference operator()(Args... args) const;
        template <class... Args>
        const_reference at(Args... args) const;
        template <class OS>
        disable_integral_t<OS, const_reference> operator[](const OS& index) const;
        template <class I>
        const_reference operator[](std::initializer_list<I> index) const;
        const_reference operator[](size_type i) const;
        template <class It>
        const_reference element(It first, It last) const;

        template <class ST>
        bool broadcast_shape(ST& shape, bool reuse_cache = false) const;

        template <class ST>
        bool is_trivial_broadcast(const ST& strides) const;

        template <class ST>
        stepper stepper_begin(const ST& shape);
        template <class ST>
        stepper stepper_end(const ST& shape, layout_type l);

        template <class ST>
        const_stepper stepper_begin(const ST& shape) const;
        template <class ST>
        const_stepper stepper_end(const ST& shape, layout_type l) const;

        template <class T = xexpression_type>
        std::enable_if_t<has_data_interface<T>::value, typename T::storage_type&>
        storage();

        template <class T = xexpression_type>
        std::enable_if_t<has_data_interface<T>::value, const typename T::storage_type&>
        storage() const;

        template <class T = xexpression_type>
        std::enable_if_t<has_data_interface<T>::value, const strides_type&>
        strides() const;

        template <class T = xexpression_type>
        std::enable_if_t<has_data_interface<T>::value, const value_type*>
        data() const;

        template <class T = xexpression_type>
        std::enable_if_t<has_data_interface<T>::value, value_type*>
        data();

        template <class T = xexpression_type>
        std::enable_if_t<has_data_interface<T>::value, const std::size_t>
        data_offset() const noexcept;

        template <class ST = self_type, class = std::enable_if_t<is_xscalar<std::decay_t<ST>>::value, int>>
        operator reference()
        {
            return (*this)();
        }

        template <class ST = self_type, class = std::enable_if_t<is_xscalar<std::decay_t<ST>>::value, int>>
        operator const_reference() const
        {
            return (*this)();
        }

        size_type underlying_size(size_type dim) const;

        xtl::xclosure_pointer<self_type&> operator&() &;
        xtl::xclosure_pointer<const self_type&> operator&() const &;
        xtl::xclosure_pointer<self_type> operator&() &&;

    private:

        // VS 2015 workaround (yes, really)
        template <std::size_t I>
        struct lesser_condition
        {
            static constexpr bool value = (I + newaxis_count_before<S...>(I + 1) < sizeof...(S));
        };

        CT m_e;
        slice_type m_slices;
        inner_shape_type m_shape;
        mutable strides_type m_strides;
        mutable bool m_strides_computed;

        strides_type compute_strides() const;

        template <typename std::decay_t<CT>::size_type... I, class... Args>
        reference access_impl(std::index_sequence<I...>, Args... args);

        template <typename std::decay_t<CT>::size_type... I, class... Args>
        const_reference access_impl(std::index_sequence<I...>, Args... args) const;

        template <typename std::decay_t<CT>::size_type I, class... Args>
        std::enable_if_t<lesser_condition<I>::value, size_type> index(Args... args) const;

        template <typename std::decay_t<CT>::size_type I, class... Args>
        std::enable_if_t<!lesser_condition<I>::value, size_type> index(Args... args) const;

        template <typename std::decay_t<CT>::size_type, class T>
        size_type sliced_access(const xslice<T>& slice) const;

        template <typename std::decay_t<CT>::size_type I, class T, class Arg, class... Args>
        size_type sliced_access(const xslice<T>& slice, Arg arg, Args... args) const;

        template <typename std::decay_t<CT>::size_type I, class T, class... Args>
        disable_xslice<T, size_type> sliced_access(const T& squeeze, Args...) const;

        using temporary_type = typename xcontainer_inner_types<self_type>::temporary_type;
        using base_index_type = xindex_type_t<shape_type>;

        template <class It>
        base_index_type make_index(It first, It last) const;

        void assign_temporary_impl(temporary_type&& tmp);

        friend class xview_semantic<xview<CT, S...>>;
    };

    template <class E, class... S>
    auto view(E&& e, S&&... slices);

    /*****************************
     * xview_stepper declaration *
     *****************************/

    namespace detail
    {
        template <class V>
        struct get_stepper_impl
        {
            using xexpression_type = typename V::xexpression_type;
            using type = typename xexpression_type::stepper;
        };

        template <class V>
        struct get_stepper_impl<const V>
        {
            using xexpression_type = typename V::xexpression_type;
            using type = typename xexpression_type::const_stepper;
        };
    }

    template <class V>
    using get_stepper = typename detail::get_stepper_impl<V>::type;

    template <bool is_const, class CT, class... S>
    class xview_stepper
    {
    public:

        using view_type = std::conditional_t<is_const,
                                             const xview<CT, S...>,
                                             xview<CT, S...>>;
        using substepper_type = get_stepper<view_type>;

        using value_type = typename substepper_type::value_type;
        using reference = typename substepper_type::reference;
        using pointer = typename substepper_type::pointer;
        using difference_type = typename substepper_type::difference_type;
        using size_type = typename view_type::size_type;

        using shape_type = typename substepper_type::shape_type;

        xview_stepper() = default;
        xview_stepper(view_type* view, substepper_type it,
                      size_type offset, bool end = false);

        reference operator*() const;

        void step(size_type dim);
        void step_back(size_type dim);
        void step(size_type dim, size_type n);
        void step_back(size_type dim, size_type n);
        void reset(size_type dim);
        void reset_back(size_type dim);

        void to_begin();
        void to_end(layout_type);

    private:

        bool is_newaxis_slice(size_type index) const noexcept;
        void to_end_impl();

        template <class F>
        void common_step(size_type dim, F f);

        template <class F>
        void common_step(size_type dim, size_type n, F f);

        template <class F>
        void common_reset(size_type dim, F f);

        view_type* p_view;
        substepper_type m_it;
        size_type m_offset;
    };

    // meta-function returning the shape type for an xview
    template <class ST, class... S>
    struct xview_shape_type
    {
        using type = ST;
    };

    template <class I, std::size_t L, class... S>
    struct xview_shape_type<std::array<I, L>, S...>
    {
        using type = std::array<I, L - integral_count<S...>() + newaxis_count<S...>()>;
    };

    namespace detail
    {
        template <class T>
        struct static_dimension
        {
            static constexpr std::ptrdiff_t value = -1;
        };

        template <class T, std::size_t N>
        struct static_dimension<std::array<T, N>>
        {
            static constexpr std::ptrdiff_t value = static_cast<std::ptrdiff_t>(N);
        };

        template <class T, std::size_t N>
        struct static_dimension<xt::const_array<T, N>>
        {
            static constexpr std::ptrdiff_t value = static_cast<std::ptrdiff_t>(N);
        };

        template <class CT, class... S>
        struct is_xscalar_impl<xview<CT, S...>>
        {
            static constexpr bool value = static_cast<std::ptrdiff_t>(integral_count<S...>()) == static_dimension<typename std::decay_t<CT>::shape_type>::value ? true : false;
        };
    }

    /************************
     * xview implementation *
     ************************/

    /**
     * @name Constructor
     */
    //@{
    /**
     * Constructs a view on the specified xexpression.
     * Users should not call directly this constructor but
     * use the view function instead.
     * @param e the xexpression to adapt
     * @param first_slice the first slice describing the view
     * @param slices the slices list describing the view
     * @sa view
     */
    template <class CT, class... S>
    template <class CTA, class FSL, class... SL>
    inline xview<CT, S...>::xview(CTA&& e, FSL&& first_slice, SL&&... slices) noexcept
        : m_e(std::forward<CTA>(e)), m_slices(std::forward<FSL>(first_slice), std::forward<SL>(slices)...),
          m_shape(xtl::make_sequence<shape_type>(m_e.dimension() - integral_count<S...>() + newaxis_count<S...>(), 0)),
          m_strides_computed(false)
    {
        auto func = [](const auto& s) noexcept { return get_size(s); };
        for (size_type i = 0; i != dimension(); ++i)
        {
            size_type index = integral_skip<S...>(i);
            m_shape[i] = index < sizeof...(S) ?
                apply<std::size_t>(index, func, m_slices) : m_e.shape()[index - newaxis_count_before<S...>(index)];
        }
    }
    //@}

    template <class CT, class... S>
    inline auto xview<CT, S...>::operator=(const xview& rhs) -> self_type&
    {
        temporary_type tmp(rhs);
        return this->assign_temporary(std::move(tmp));
    }

    /**
     * @name Extended copy semantic
     */
    //@{
    /**
     * The extended assignment operator.
     */
    template <class CT, class... S>
    template <class E>
    inline auto xview<CT, S...>::operator=(const xexpression<E>& e) -> self_type&
    {
        return semantic_base::operator=(e);
    }
    //@}

    template <class CT, class... S>
    template <class E>
    inline auto xview<CT, S...>::operator=(const E& e) -> disable_xexpression<E, self_type>&
    {
        std::fill(this->begin(), this->end(), e);
        return *this;
    }

    /**
     * @name Size and shape
     */
    //@{
    /**
     * Returns the size of the expression.
     */
    template <class CT, class... S>
    inline auto xview<CT, S...>::size() const noexcept -> size_type
    {
        return compute_size(shape());
    }

    /**
     * Returns the number of dimensions of the view.
     */
    template <class CT, class... S>
    inline auto xview<CT, S...>::dimension() const noexcept -> size_type
    {
        return m_shape.size();
    }

    /**
     * Returns the shape of the view.
     */
    template <class CT, class... S>
    inline auto xview<CT, S...>::shape() const noexcept -> const inner_shape_type&
    {
        return m_shape;
    }

    /**
     * Returns the slices of the view.
     */
    template <class CT, class... S>
    inline auto xview<CT, S...>::slices() const noexcept -> const slice_type&
    {
        return m_slices;
    }

    /**
     * Returns the slices of the view.
     */
    template <class CT, class... S>
    inline layout_type xview<CT, S...>::layout() const noexcept
    {
        return do_strides_match(shape(), strides(), m_e.layout()) ? m_e.layout() : layout_type::dynamic;
    }
    //@}

    /**
     * @name Data
     */
    //@{
    /**
     * Returns a reference to the element at the specified position in the view.
     * @param args a list of indices specifying the position in the view. Indices
     * must be unsigned integers, the number of indices should be equal or greater
     * than the number of dimensions of the view.
     */
    template <class CT, class... S>
    template <class... Args>
    inline auto xview<CT, S...>::operator()(Args... args) -> reference
    {
        // The static cast prevents the compiler from instantiating the template methods with signed integers,
        // leading to warning about signed/unsigned conversions in the deeper layers of the access methods
        return access_impl(std::make_index_sequence<(sizeof...(Args) + integral_count<S...>() > newaxis_count<S...>() ?
                                                        sizeof...(Args) + integral_count<S...>() - newaxis_count<S...>() :
                                                        0)>(),
                           static_cast<size_type>(args)...);
    }

    /**
     * Returns a reference to the element at the specified position in the expression,
     * after dimension and bounds checking.
     * @param args a list of indices specifying the position in the function. Indices
     * must be unsigned integers, the number of indices should be equal to the number of dimensions
     * of the expression.
     * @exception std::out_of_range if the number of argument is greater than the number of dimensions
     * or if indices are out of bounds.
     */
    template <class CT, class... S>
    template <class... Args>
    inline auto xview<CT, S...>::at(Args... args) -> reference
    {
        check_access(shape(), static_cast<size_type>(args)...);
        return this->operator()(args...);
    }

    template <class CT, class... S>
    template <class OS>
    inline auto xview<CT, S...>::operator[](const OS& index)
        -> disable_integral_t<OS, reference>
    {
        return element(index.cbegin(), index.cend());
    }

    template <class CT, class... S>
    template <class I>
    inline auto xview<CT, S...>::operator[](std::initializer_list<I> index)
        -> reference
    {
        return element(index.begin(), index.end());
    }

    template <class CT, class... S>
    inline auto xview<CT, S...>::operator[](size_type i) -> reference
    {
        return operator()(i);
    }

    template <class CT, class... S>
    template <class It>
    inline auto xview<CT, S...>::element(It first, It last) -> reference
    {
        // TODO: avoid memory allocation
        auto index = make_index(first, last);
        return m_e.element(index.cbegin(), index.cend());
    }

    /**
     * Returns a constant reference to the element at the specified position in the view.
     * @param args a list of indices specifying the position in the view. Indices must be
     * unsigned integers, the number of indices should be equal or greater than the number
     * of dimensions of the view.
     */
    template <class CT, class... S>
    template <class... Args>
    inline auto xview<CT, S...>::operator()(Args... args) const -> const_reference
    {
        // The static cast prevents the compiler from instantiating the template methods with signed integers,
        // leading to warning about signed/unsigned conversions in the deeper layers of the access methods
        return access_impl(std::make_index_sequence<(sizeof...(Args) + integral_count<S...>() > newaxis_count<S...>() ?
                                                        sizeof...(Args) + integral_count<S...>() - newaxis_count<S...>() :
                                                        0)>(),
                           static_cast<size_type>(args)...);
    }

    /**
     * Returns a constant reference to the element at the specified position in the expression,
     * after dimension and bounds checking.
     * @param args a list of indices specifying the position in the function. Indices
     * must be unsigned integers, the number of indices should be equal to the number of dimensions
     * of the expression.
     * @exception std::out_of_range if the number of argument is greater than the number of dimensions
     * or if indices are out of bounds.
     */
    template <class CT, class... S>
    template <class... Args>
    inline auto xview<CT, S...>::at(Args... args) const -> const_reference
    {
        check_access(shape(), static_cast<size_type>(args)...);
        return this->operator()(args...);
    }

    template <class CT, class... S>
    template <class OS>
    inline auto xview<CT, S...>::operator[](const OS& index) const
        -> disable_integral_t<OS, const_reference>
    {
        return element(index.cbegin(), index.cend());
    }

    template <class CT, class... S>
    template <class I>
    inline auto xview<CT, S...>::operator[](std::initializer_list<I> index) const
        -> const_reference
    {
        return element(index.begin(), index.end());
    }

    template <class CT, class... S>
    inline auto xview<CT, S...>::operator[](size_type i) const -> const_reference
    {
        return operator()(i);
    }

    template <class CT, class... S>
    template <class It>
    inline auto xview<CT, S...>::element(It first, It last) const -> const_reference
    {
        // TODO: avoid memory allocation
        auto index = make_index(first, last);
        return m_e.element(index.cbegin(), index.cend());
    }

    /**
     * Returns the data holder of the underlying container (only if the view is on a realized
     * container). ``xt::eval`` will make sure that the underlying xexpression is
     * on a realized container.
     */
    template <class CT, class... S>
    template <class T>
    inline auto xview<CT, S...>::storage() ->
        std::enable_if_t<has_data_interface<T>::value, typename T::storage_type&>
    {
        return m_e.storage();
    }

    template <class CT, class... S>
    template <class T>
    inline auto xview<CT, S...>::storage() const ->
        std::enable_if_t<has_data_interface<T>::value, const typename T::storage_type&>
    {
        return m_e.storage();
    }

    /**
     * Return the strides for the underlying container of the view.
     */
    template <class CT, class... S>
    template <class T>
    inline auto xview<CT, S...>::strides() const ->
        std::enable_if_t<has_data_interface<T>::value, const strides_type&>
    {
        if (!m_strides_computed)
        {
            m_strides = compute_strides();
            m_strides_computed = true;
        }
        return m_strides;
    }

    /**
     * Return the pointer to the underlying buffer.
     */
    template <class CT, class... S>
    template <class T>
    inline auto xview<CT, S...>::data() const ->
        std::enable_if_t<has_data_interface<T>::value, const value_type*>
    {
        return m_e.data();
    }

    template <class CT, class... S>
    template <class T>
    inline auto xview<CT, S...>::data() ->
        std::enable_if_t<has_data_interface<T>::value, value_type*>
    {
        return m_e.data();
    }

    /**
     * Return the offset to the first element of the view in the underlying container.
     */
    template <class CT, class... S>
    template <class T>
    inline auto xview<CT, S...>::data_offset() const noexcept ->
        std::enable_if_t<has_data_interface<T>::value, const std::size_t>
    {
        auto func = [](const auto& s) { return xt::value(s, 0); };
        typename T::size_type offset = m_e.data_offset();

        for (size_type i = 0; i < sizeof...(S); ++i)
        {
            size_type s = apply<size_type>(i, func, m_slices) * m_e.strides()[i];
            offset += s;
        }
        return offset;
    }
    //@}

    template <class CT, class... S>
    inline auto xview<CT, S...>::underlying_size(size_type dim) const -> size_type
    {
        return m_e.shape()[dim];
    }

    template <class CT, class... S>
    inline auto xview<CT, S...>::operator&() & -> xtl::xclosure_pointer<self_type&>
    {
        return xtl::closure_pointer(*this);
    }

    template <class CT, class... S>
    inline auto xview<CT, S...>::operator&() const & -> xtl::xclosure_pointer<const self_type&>
    {
        return xtl::closure_pointer(*this);
    }

    template <class CT, class... S>
    inline auto xview<CT, S...>::operator&() && -> xtl::xclosure_pointer<self_type>
    {
        return xtl::closure_pointer(std::move(*this));
    }

    /**
     * @name Broadcasting
     */
    //@{
    /**
     * Broadcast the shape of the view to the specified parameter.
     * @param shape the result shape
     * @return a boolean indicating whether the broadcasting is trivial
     */
    template <class CT, class... S>
    template <class ST>
    inline bool xview<CT, S...>::broadcast_shape(ST& shape, bool) const
    {
        return xt::broadcast_shape(m_shape, shape);
    }

    /**
     * Compares the specified strides with those of the view to see whether
     * the broadcasting is trivial.
     * @return a boolean indicating whether the broadcasting is trivial
     */
    template <class CT, class... S>
    template <class ST>
    inline bool xview<CT, S...>::is_trivial_broadcast(const ST& /*strides*/) const
    {
        return false;
    }
    //@}

    template <class CT, class... S>
    inline auto xview<CT, S...>::compute_strides() const -> strides_type
    {
        strides_type strides = xtl::make_sequence<strides_type>(dimension(), 0);

        auto func = [](const auto& s) { return xt::step_size(s); };

        for (size_type i = 0; i != dimension(); ++i)
        {
            size_type index = integral_skip<S...>(i);
            strides[i] = index < sizeof...(S) ?
                apply<size_type>(index, func, m_slices) * m_e.strides()[index - newaxis_count_before<S...>(index)] :
                m_e.strides()[index - newaxis_count_before<S...>(index)];
            // adapt strides for shape[i] == 1 to make consistent with rest of xtensor
            if (shape()[i] == 1)
            {
                strides[i] = 0;
            }
        }

        return strides;
    }

    template <class CT, class... S>
    template <typename std::decay_t<CT>::size_type... I, class... Args>
    inline auto xview<CT, S...>::access_impl(std::index_sequence<I...>, Args... args) -> reference
    {
        return m_e(index<I>(args...)...);
    }

    template <class CT, class... S>
    template <typename std::decay_t<CT>::size_type... I, class... Args>
    inline auto xview<CT, S...>::access_impl(std::index_sequence<I...>, Args... args) const -> const_reference
    {
        return m_e(index<I>(args...)...);
    }

    template <class CT, class... S>
    template <typename std::decay_t<CT>::size_type I, class... Args>
    inline auto xview<CT, S...>::index(Args... args) const -> std::enable_if_t<lesser_condition<I>::value, size_type>
    {
        return sliced_access<I - integral_count_before<S...>(I) + newaxis_count_before<S...>(I + 1)>
            (std::get<I + newaxis_count_before<S...>(I + 1)>(m_slices), args...);
    }

    template <class CT, class... S>
    template <typename std::decay_t<CT>::size_type I, class... Args>
    inline auto xview<CT, S...>::index(Args... args) const -> std::enable_if_t<!lesser_condition<I>::value, size_type>
    {
        return argument<I - integral_count<S...>() + newaxis_count<S...>()>(args...);
    }

    template <class CT, class... S>
    template <typename std::decay_t<CT>::size_type I, class T>
    inline auto xview<CT, S...>::sliced_access(const xslice<T>& slice) const -> size_type
    {
        return slice.derived_cast()(0);
    }

    template <class CT, class... S>
    template <typename std::decay_t<CT>::size_type I, class T, class Arg, class... Args>
    inline auto xview<CT, S...>::sliced_access(const xslice<T>& slice, Arg arg, Args... args) const -> size_type
    {
        using ST = typename T::size_type;
        return static_cast<size_type>(slice.derived_cast()(argument<I>(static_cast<ST>(arg), static_cast<ST>(args)...)));
    }

    template <class CT, class... S>
    template <typename std::decay_t<CT>::size_type I, class T, class... Args>
    inline auto xview<CT, S...>::sliced_access(const T& squeeze, Args...) const -> disable_xslice<T, size_type>
    {
        return static_cast<size_type>(squeeze);
    }

    template <class CT, class... S>
    template <class It>
    inline auto xview<CT, S...>::make_index(It first, It last) const -> base_index_type
    {
        auto index = xtl::make_sequence<typename xexpression_type::shape_type>(m_e.dimension(), 0);
        auto func1 = [&first](const auto& s) {
            return get_slice_value(s, first);
        };
        auto func2 = [](const auto& s) {
            return xt::value(s, 0);
        };
        for (size_type i = 0; i != m_e.dimension(); ++i)
        {
            size_type k = newaxis_skip<S...>(i);
            std::advance(first, k - i);
            if (first != last)
            {
                index[i] = k < sizeof...(S) ?
                    apply<size_type>(k, func1, m_slices) : *first++;
            }
            else
            {
                index[i] = k < sizeof...(S) ?
                    apply<size_type>(k, func2, m_slices) : 0;
            }
        }
        return index;
    }

    template <class CT, class... S>
    inline void xview<CT, S...>::assign_temporary_impl(temporary_type&& tmp)
    {
        std::copy(tmp.cbegin(), tmp.cend(), this->begin());
    }

    namespace detail
    {
        template <class E, class... S>
        inline std::size_t get_underlying_shape_index(std::size_t I)
        {
            return I - newaxis_count_before<get_slice_type<E, S>...>(I);
        }

        template <class... S>
        struct check_slice;

        template <>
        struct check_slice<>
        {
            using type = void_t<>;
        };

        template <class S, class... SL>
        struct check_slice<S, SL...>
        {
            static_assert(!std::is_same<S, xellipsis_tag>::value, "ellipsis not supported vith xview");
            using type = typename check_slice<SL...>::type;
        };

        template <class E, std::size_t... I, class... S>
        inline auto make_view_impl(E&& e, std::index_sequence<I...>, S&&... slices)
        {
            // Checks that no ellipsis slice is used
            using type = typename check_slice<S...>::type;
            using view_type = xview<xtl::closure_type_t<E>, get_slice_type<std::decay_t<E>, S>...>;
            return view_type(std::forward<E>(e),
                get_slice_implementation(e, std::forward<S>(slices), get_underlying_shape_index<std::decay_t<E>, S...>(I))...
            );
        }
    }

    /**
     * Constructs and returns a view on the specified xexpression. Users
     * should not directly construct the slices but call helper functions
     * instead.
     * @param e the xexpression to adapt
     * @param slices the slices list describing the view
     * @sa range, all, newaxis
     */
    template <class E, class... S>
    inline auto view(E&& e, S&&... slices)
    {
        return detail::make_view_impl(std::forward<E>(e), std::make_index_sequence<sizeof...(S)>(), std::forward<S>(slices)...);
    }

    /***************
     * stepper api *
     ***************/

    template <class CT, class... S>
    template <class ST>
    inline auto xview<CT, S...>::stepper_begin(const ST& shape) -> stepper
    {
        size_type offset = shape.size() - dimension();
        return stepper(this, m_e.stepper_begin(m_e.shape()), offset);
    }

    template <class CT, class... S>
    template <class ST>
    inline auto xview<CT, S...>::stepper_end(const ST& shape, layout_type l) -> stepper
    {
        size_type offset = shape.size() - dimension();
        return stepper(this, m_e.stepper_end(m_e.shape(), l), offset, true);
    }

    template <class CT, class... S>
    template <class ST>
    inline auto xview<CT, S...>::stepper_begin(const ST& shape) const -> const_stepper
    {
        size_type offset = shape.size() - dimension();
        const xexpression_type& e = m_e;
        return const_stepper(this, e.stepper_begin(m_e.shape()), offset);
    }

    template <class CT, class... S>
    template <class ST>
    inline auto xview<CT, S...>::stepper_end(const ST& shape, layout_type l) const -> const_stepper
    {
        size_type offset = shape.size() - dimension();
        const xexpression_type& e = m_e;
        return const_stepper(this, e.stepper_end(m_e.shape(), l), offset, true);
    }

    /********************************
     * xview_stepper implementation *
     ********************************/

    template <bool is_const, class CT, class... S>
    inline xview_stepper<is_const, CT, S...>::xview_stepper(view_type* view, substepper_type it,
                                                            size_type offset, bool end)
        : p_view(view), m_it(it), m_offset(offset)
    {
        if (!end)
        {
            auto func = [](const auto& s) { return xt::value(s, 0); };
            for (size_type i = 0; i < sizeof...(S); ++i)
            {
                if (!is_newaxis_slice(i))
                {
                    size_type s = apply<size_type>(i, func, p_view->slices());
                    size_type index = i - newaxis_count_before<S...>(i);
                    m_it.step(index, s);
                }
            }
        }
        else
        {
            to_end_impl();
        }
    }

    template <bool is_const, class CT, class... S>
    inline auto xview_stepper<is_const, CT, S...>::operator*() const -> reference
    {
        return *m_it;
    }

    template <bool is_const, class CT, class... S>
    inline void xview_stepper<is_const, CT, S...>::step(size_type dim)
    {
        auto func = [this](size_type index, size_type offset) { m_it.step(index, offset); };
        common_step(dim, func);
    }

    template <bool is_const, class CT, class... S>
    inline void xview_stepper<is_const, CT, S...>::step_back(size_type dim)
    {
        auto func = [this](size_type index, size_type offset) { m_it.step_back(index, offset); };
        common_step(dim, func);
    }

    template <bool is_const, class CT, class... S>
    inline void xview_stepper<is_const, CT, S...>::step(size_type dim, size_type n)
    {
        auto func = [this](size_type index, size_type offset) { m_it.step(index, offset); };
        common_step(dim, n, func);
    }

    template <bool is_const, class CT, class... S>
    inline void xview_stepper<is_const, CT, S...>::step_back(size_type dim, size_type n)
    {
        auto func = [this](size_type index, size_type offset) { m_it.step_back(index, offset); };
        common_step(dim, n, func);
    }

    template <bool is_const, class CT, class... S>
    inline void xview_stepper<is_const, CT, S...>::reset(size_type dim)
    {
        auto func = [this](size_type index, size_type offset) { m_it.step_back(index, offset); };
        common_reset(dim, func);
    }

    template <bool is_const, class CT, class... S>
    inline void xview_stepper<is_const, CT, S...>::reset_back(size_type dim)
    {
        auto func = [this](size_type index, size_type offset) { m_it.step(index, offset); };
        common_reset(dim, func);
    }

    template <bool is_const, class CT, class... S>
    inline void xview_stepper<is_const, CT, S...>::to_begin()
    {
        m_it.to_begin();
    }

    template <bool is_const, class CT, class... S>
    inline void xview_stepper<is_const, CT, S...>::to_end(layout_type l)
    {
        m_it.to_end(l);
        to_end_impl();
    }

    template <bool is_const, class CT, class... S>
    inline bool xview_stepper<is_const, CT, S...>::is_newaxis_slice(size_type index) const noexcept
    {
        // A bit tricky but avoids a lot of template instantiations
        return newaxis_count_before<S...>(index + 1) != newaxis_count_before<S...>(index);
    }

    template <bool is_const, class CT, class... S>
    inline void xview_stepper<is_const, CT, S...>::to_end_impl()
    {
        auto func = [](const auto& s) { return xt::value(s, get_size(s) - 1); };
        for (size_type i = 0; i < sizeof...(S); ++i)
        {
            if (!is_newaxis_slice(i))
            {
                size_type s = apply<size_type>(i, func, p_view->slices());
                size_type index = i - newaxis_count_before<S...>(i);
                s = p_view->underlying_size(index) - 1 - s;
                m_it.step_back(index, s);
            }
        }
    }

    template <bool is_const, class CT, class... S>
    template <class F>
    void xview_stepper<is_const, CT, S...>::common_step(size_type dim, F f)
    {
        if (dim >= m_offset)
        {
            auto func = [](const auto& s) noexcept { return step_size(s); };
            size_type index = integral_skip<S...>(dim);
            if (!is_newaxis_slice(index))
            {
                size_type step_size = index < sizeof...(S) ?
                    apply<size_type>(index, func, p_view->slices()) : 1;
                index -= newaxis_count_before<S...>(index);
                f(index, step_size);
            }
        }
    }

    template <bool is_const, class CT, class... S>
    template <class F>
    void xview_stepper<is_const, CT, S...>::common_step(size_type dim, size_type n, F f)
    {
        if (dim >= m_offset)
        {
            auto func = [](const auto& s) noexcept { return step_size(s); };
            size_type index = integral_skip<S...>(dim);
            if (!is_newaxis_slice(index))
            {
                size_type step_size = index < sizeof...(S) ?
                    apply<size_type>(index, func, p_view->slices()) : 1;
                index -= newaxis_count_before<S...>(index);
                f(index, step_size * n);
            }
        }
    }

    template <bool is_const, class CT, class... S>
    template <class F>
    void xview_stepper<is_const, CT, S...>::common_reset(size_type dim, F f)
    {
        auto size_func = [](const auto& s) noexcept { return get_size(s); };
        auto step_func = [](const auto& s) noexcept { return step_size(s); };
        size_type index = integral_skip<S...>(dim);
        if (!is_newaxis_slice(index))
        {
            size_type size = index < sizeof...(S) ? apply<size_type>(index, size_func, p_view->slices()) : p_view->shape()[dim];
            if (size != 0)
            {
                size = size - 1;
            }
            size_type step_size = index < sizeof...(S) ? apply<size_type>(index, step_func, p_view->slices()) : 1;
            index -= newaxis_count_before<S...>(index);
            f(index, step_size * size);
        }
    }
}

#endif
