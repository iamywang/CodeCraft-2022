#pragma once
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <utility>

namespace MyUtils
{
namespace DataType
{

namespace base
{
template < typename T > struct inner_struct
{
    size_t m_capacity; //容量
    size_t m_size;     //当前数据占用的空间
    T *m_data;

  public:
    inner_struct( size_t data_len = 0UL, size_t capacity = 512UL )
        : m_capacity( capacity ), m_size( data_len ), m_data( nullptr )
    {
        m_capacity = __changeValue( m_capacity > m_size ? m_capacity : m_size );
        __resize( );
    }

    inner_struct( const T *data, const size_t len )
        : m_capacity( 0 ), m_size( len ), m_data( nullptr )
    {
        m_capacity = __changeValue( m_size );
        __resize( );
        memcpy( m_data, data, len * sizeof(T));
        m_size = len;
    }

    ~inner_struct( )
    {
        if ( m_data && m_capacity > 0 )
        {
            delete[] m_data;
            m_data = nullptr;
        }
        m_capacity = 0;
        m_size = 0;
    }

    inline void copy( const T *data, const size_t size )
    {
        if ( m_data == data )
            return;
        if ( size > m_capacity )
        {
            m_capacity = __changeValue( size );
            __resize( );
        }
        if ( data == nullptr )
        {
            m_size = 0;
            return;
        }
        else
        {
            m_size = size;
            memcpy( m_data, data, size * sizeof(T) );
        }
        return;
    }

    inline inner_struct *slice( size_t start, size_t end )
    {
        if ( start > end || start > m_size )
        {
            return nullptr;
        }

        if ( end > m_size )
        {
            end = m_size;
        }

        inner_struct *ret = new inner_struct( m_data + start, end - start );
        return ret;
    }

    /**
     * @brief append
     *
     * @param [in] data
     * @param [in] size 数组长度
     */
    inline void append( const T *data, size_t size )
    {
        if ( size > m_capacity - m_size )
        {
            m_capacity = __changeValue( m_size + size );
            __resize( );
        }
        memcpy( m_data + m_size, data, size * sizeof(T));
        m_size += size;
        return;
    }

    /**
     * @brief append
     *
     * @param [in] data1 数组1
     * @param [in] size1 数组1长度
     * @param [in] data2 数组2
     * @param [in] size2 数组2长度
     * @return inner_struct*
     */
    inline static inner_struct *append( const T *data1, const size_t size1,
                                        const T *data2, const size_t size2 )
    {
        inner_struct *ret = new inner_struct( size1 + size2 );
        ret->m_size = size1 + size2;
        memcpy( ret->m_data, data1, size1 * sizeof(T) );
        memcpy( ret->m_data + size1, data2, size2 * sizeof(T) );
        return ret;
    }

    inline size_t size( ) const { return m_size; }
    inline T *data( ) const { return m_data; }
    inline void clear( ) { m_size = 0; }
    inline size_t capacity( ) const { return m_capacity; }

  private:
    inline size_t __changeValue( size_t size )
    {
        size_t e = size_t( log2( size ) );
        size_t ret = size_t( 1 ) << e;
        if ( ret < size )
            ret <<= 1;
        return ret;
    }

    inline void __resize( )
    {
        T *tmp = new T[ m_capacity ];
        if ( m_size > 0 && m_data != nullptr )
            memcpy( tmp, m_data, m_size * sizeof(T) );
        if ( m_data )
        {
            delete[] m_data;
            m_data = nullptr;
        }
        m_data = tmp;
        return;
    }
};
} // namespace base

template < typename T > class Array
{
  private:
    typedef base::inner_struct< T > inner_struct;
    std::shared_ptr< inner_struct > m_inner_data;

  private:
    Array( inner_struct *inner_struct )
        : m_inner_data( inner_struct, []( base::inner_struct< T > *ptr ) {
              if ( ptr )
                  delete ptr;
              ptr = nullptr;
          } )
    {
    }

  public:
    Array( size_t data_len = size_t(0), size_t capacity = 512UL )
        : m_inner_data( new inner_struct( data_len, capacity ),
                        []( inner_struct *ptr ) {
                            if ( ptr )
                                delete ptr;
                            ptr = nullptr;
                        } )
    {
    }

    Array( Array &&other ) : m_inner_data( std::move( other.m_inner_data ) ) {}

    Array &operator=( const Array &other ) = default;
    Array( const Array &other ) = default;

    /**
     * @brief deep copy constructor
     * @param [in] data 数据指针
     * @param [in] len 数据大小
     */
    Array( const T *data, const size_t &len )
        : m_inner_data( new inner_struct( data, len ), []( inner_struct *ptr ) {
              if ( ptr )
                  delete ptr;
              ptr = nullptr;
          } )
    {
    }

    ~Array( ) {}

  public:
    inline bool exist( ) const { return m_inner_data != nullptr; }

    /**
     * @brief 允许用户自行修改数据长度，但是最大不会超过其容量。
     *
     * @param [in] data_len 数据长度
     */
    inline void setDataLen( size_t data_len )
    {
        if ( m_inner_data == nullptr )
            return;
        m_inner_data->m_size = data_len > m_inner_data->m_capacity
                                   ? m_inner_data->m_capacity
                                   : data_len;
        return;
    }

    inline void reset( )
    {
        setDataLen( 0 );
        return;
    }

    inline const T *data( ) const
    {
        return this->m_inner_data ? this->m_inner_data->data( ) : nullptr;
    }

    inline T *data( )
    {
        return this->m_inner_data ? this->m_inner_data->data( ) : nullptr;
    }

    inline size_t size( ) const
    {
        return this->m_inner_data ? this->m_inner_data->size( ) : 0;
    }
    inline size_t capacity( ) const
    {
        return this->m_inner_data ? this->m_inner_data->capacity( ) : 0;
    }

    inline T operator[]( const size_t index ) const
    {
        return this->m_inner_data->data( )[ index ];
    }

    inline T &operator[]( const size_t index )
    {
        return this->m_inner_data->data( )[ index ];
    }

  public:
    /**
     * @brief 将自身拷贝一份
     * @return Array
     */
    inline Array clone( )
    {
        return std::move(
            Array( m_inner_data->data( ), m_inner_data->size( ) ) );
    }

    /**
     * @brief 将other的数据拷贝到自身，这会覆盖自身已有的数据
     * @param [in] other
     * @return void
     */
    inline void copy( const Array &other )
    {
        m_inner_data->copy( other.m_inner_data->data( ),
                            other.m_inner_data->size( ) );
    }

    /**
     * @brief 拷贝动作会覆盖掉原有数据
     * @param [in] data 数据指针
     * @param [in] len 数据长度
     */
    inline void copy( const T *data, const size_t len )
    {
        m_inner_data->copy( data, len );
        return;
    }

    /**
     * @brief 将数据追加到原有数据之后
     * @param [in] data 数据指针
     * @param [in] len 数据长度
     */
    void append( const T *data, const size_t len )
    {
        m_inner_data->append( data, len );
        return;
    }

    /**
     * @brief append
     * @param [in] value
     * @return void
     */
    inline void append( T &value )
    {
        this->append( &value, sizeof( T ) );
        return;
    }

    inline void append( T &&value )
    {
        T val{value};
        this->append( &val, 1 );
        return;
    }

    /**
     * @brief append
     * 
     * @param [in] data1 数组1
     * @param [in] len1 数组1长度
     * @param [in] data2 数组2
     * @param [in] len2 数组2长度
     * @return Array 
     */
    static Array append( const T *data1, const size_t len1,
                         const T *data2, const size_t len2 )
    {
        return inner_struct::append( data1, len1, data2, len2 );
    }

    inline void swap( Array &other )
    {
        std::swap( m_inner_data, other.m_inner_data );
    }

    /**
     * @brief 清除已经原先保存的数据
     */
    inline void clear( ) { this->m_inner_data->clear( ); }

    /**
     * @brief 提取[start_ops,
     * end_ops)的数据。如果出现错误则返回的ByteArray的size为0
     */
    Array slice( size_t start_ops, size_t end_ops = SIZE_MAX )
    {
        return m_inner_data->slice( start_ops, end_ops );
    }
};

} // namespace DataType

} // namespace MyUtils