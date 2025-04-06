#pragma once

#include <array>
#include <span>
#include <string_view>
#include <concepts>
#include <variant>
#include <format>
#include <sstream>
#include <type_traits>
#include <cassert>
#include <optional>
#include <cstdint>

// Optional Qt support
#ifdef SQLQUERYBUILDER_USE_QT
#include <QDateTime>
#include <QString>
#endif

namespace sql {
inline namespace v1 {

// Configuration
struct DefaultConfig {
    static constexpr size_t MaxColumns = 32;
    static constexpr size_t MaxConditions = 16;
    static constexpr size_t MaxJoins = 4;
    static constexpr size_t MaxOrderBy = 8;
    static constexpr size_t MaxGroupBy = 8;
    static constexpr bool ThrowOnError = false;
};

// Error handling
struct QueryError {
    enum class Code {
        None,
        TooManyColumns,
        TooManyConditions,
        TooManyJoins,
        EmptyTable,
        InvalidColumn,
        InvalidCondition,
        TooManyOrderBy,
        TooManyGroupBy
    };

    Code code{Code::None};
    std::string_view message;

    QueryError() = default;
    QueryError(Code c, std::string_view msg) : code(c), message(msg) {}

    explicit operator bool() const { return code != Code::None; }
};

template<typename T>
class Result {
    std::variant<T, QueryError> value_;

public:
    Result(T value) : value_(std::move(value)) {}
    Result(QueryError error) : value_(std::move(error)) {}

    bool hasError() const { return std::holds_alternative<QueryError>(value_); }
    const QueryError& error() const { return std::get<QueryError>(value_); }
    const T& value() const { return std::get<T>(value_); }

    explicit operator bool() const { return !hasError(); }
};

template<typename T>
struct is_string_literal : std::false_type {};

template<std::size_t N>
struct is_string_literal<const char[N]> : std::true_type {};

template<std::size_t N>
struct is_string_literal<char[N]> : std::true_type {};

// Qt support
#ifdef SQLQUERYBUILDER_USE_QT
template<typename T>
struct is_qt_type : std::false_type {};

template<>
struct is_qt_type<QString> : std::true_type {};

template<>
struct is_qt_type<QDateTime> : std::true_type {};

template<>
struct is_qt_type<QString&> : std::true_type {};

template<>
struct is_qt_type<const QString&> : std::true_type {};

template<>
struct is_qt_type<QDateTime&> : std::true_type {};

template<>
struct is_qt_type<const QDateTime&> : std::true_type {};
#endif

template<typename T>
concept SqlCompatible =
    std::is_integral_v<std::remove_cvref_t<T>> ||
    std::is_floating_point_v<std::remove_cvref_t<T>> ||
    std::is_enum_v<std::remove_cvref_t<T>> ||
    std::same_as<std::remove_cvref_t<T>, std::string_view> ||
    std::same_as<std::remove_cvref_t<T>, std::string> ||
    is_string_literal<std::remove_cvref_t<T>>::value ||
#ifdef SQLQUERYBUILDER_USE_QT
    is_qt_type<std::remove_cvref_t<T>>::value ||
#endif
    std::same_as<std::remove_cvref_t<T>, bool>;

// Forward declarations
template<typename Config = DefaultConfig>
class SqlValue;

template<typename Config = DefaultConfig>
class Condition;

template<typename Config = DefaultConfig>
class Column;

// Table class with config awareness
template<typename Config = DefaultConfig>
class Table {
private:
    std::string_view name_;

public:
    constexpr explicit Table(std::string_view name) : name_(name) {}

    [[nodiscard]] constexpr std::string_view name() const { return name_; }

    operator std::string_view() const { return name_; }
};

// TypedColumn class with config awareness
template<typename T, typename Config = DefaultConfig>
class TypedColumn {
private:
    std::string_view table_;
    std::string_view name_;

public:
    using value_type = T;
    using config_type = Config;

    constexpr TypedColumn(std::string_view table, std::string_view name)
        : table_(table), name_(name) {}

    [[nodiscard]] constexpr std::string_view tableName() const { return table_; }
    [[nodiscard]] constexpr std::string_view name() const { return name_; }

    // Generate qualified name (table.column)
    [[nodiscard]] std::string qualifiedName() const {
        return std::string(table_) + "." + std::string(name_);
    }

    operator std::string_view() const { return name_; }

    // Condition generation methods
    [[nodiscard]] Condition<Config> isNull() const;
    [[nodiscard]] Condition<Config> isNotNull() const;

    template<SqlCompatible U>
    [[nodiscard]] Condition<Config> eq(U&& value) const;

    template<typename OtherType, typename OtherConfig>
    [[nodiscard]] Condition<Config> eq(const TypedColumn<OtherType, OtherConfig>& other) const;

    template<SqlCompatible U>
    [[nodiscard]] Condition<Config> ne(U&& value) const;

    template<typename OtherType, typename OtherConfig>
    [[nodiscard]] Condition<Config> ne(const TypedColumn<OtherType, OtherConfig>& other) const;

    template<SqlCompatible U>
    [[nodiscard]] Condition<Config> lt(U&& value) const;

    template<typename OtherType, typename OtherConfig>
    [[nodiscard]] Condition<Config> lt(const TypedColumn<OtherType, OtherConfig>& other) const;

    template<SqlCompatible U>
    [[nodiscard]] Condition<Config> le(U&& value) const;

    template<typename OtherType, typename OtherConfig>
    [[nodiscard]] Condition<Config> le(const TypedColumn<OtherType, OtherConfig>& other) const;

    template<SqlCompatible U>
    [[nodiscard]] Condition<Config> gt(U&& value) const;

    template<typename OtherType, typename OtherConfig>
    [[nodiscard]] Condition<Config> gt(const TypedColumn<OtherType, OtherConfig>& other) const;

    template<SqlCompatible U>
    [[nodiscard]] Condition<Config> ge(U&& value) const;

    template<typename OtherType, typename OtherConfig>
    [[nodiscard]] Condition<Config> ge(const TypedColumn<OtherType, OtherConfig>& other) const;

    [[nodiscard]] Condition<Config> like(std::string_view pattern) const;
    [[nodiscard]] Condition<Config> notLike(std::string_view pattern) const;

    template<SqlCompatible T1, SqlCompatible T2>
    [[nodiscard]] Condition<Config> between(T1&& start, T2&& end) const;
};

// SqlValue class with type-safe storage
template<typename Config>
class SqlValue {
public:
    using StorageType = std::variant<
        std::monostate,  // For NULL
        int64_t,
        double,
        bool,
        std::string_view
#ifdef SQLQUERYBUILDER_USE_QT
        ,QString
        ,QDateTime
#endif
        >;

private:
    StorageType storage_;

public:
    SqlValue() = default;

    template<SqlCompatible T>
    explicit SqlValue(T&& value) {
        if constexpr(std::is_integral_v<std::remove_cvref_t<T>> && !std::is_same_v<std::remove_cvref_t<T>, bool>) {
            storage_ = static_cast<int64_t>(value);
        } else if constexpr(std::is_floating_point_v<std::remove_cvref_t<T>>) {
            storage_ = static_cast<double>(value);
        } else if constexpr(std::is_same_v<std::remove_cvref_t<T>, bool>) {
            storage_ = value;
        } else if constexpr(std::is_same_v<std::remove_cvref_t<T>, std::string_view>) {
            storage_ = value;
        } else if constexpr(is_string_literal<std::remove_cvref_t<T>>::value) {
            storage_ = std::string_view(value);
#ifdef SQLQUERYBUILDER_USE_QT
        } else if constexpr(std::is_same_v<std::remove_cvref_t<T>, QString>) {
            storage_ = value;
        } else if constexpr(std::is_same_v<std::remove_cvref_t<T>, QDateTime>) {
            storage_ = value;
#endif
        } else if constexpr(std::is_enum_v<std::remove_cvref_t<T>>) {
            storage_ = static_cast<int64_t>(value);
        }
    }

    [[nodiscard]] std::string toSqlString() const {
        return std::visit([](const auto& value) -> std::string {
            using T = std::decay_t<decltype(value)>;
            if constexpr(std::is_same_v<T, std::monostate>) {
                return "NULL";
            } else if constexpr(std::is_integral_v<T>) {
                return std::to_string(value);
            } else if constexpr(std::is_floating_point_v<T>) {
                return std::to_string(value);
            } else if constexpr(std::is_same_v<T, bool>) {
                return value ? "1" : "0";
            } else if constexpr(std::is_same_v<T, std::string_view>) {
                std::string escaped;
                escaped.reserve(value.size() + 2);
                escaped.push_back('\'');

                for (char c : value) {
                    if (c == '\'') {
                        escaped.append("''");
                    } else {
                        escaped.push_back(c);
                    }
                }

                escaped.push_back('\'');
                return escaped;
#ifdef SQLQUERYBUILDER_USE_QT
            } else if constexpr(std::is_same_v<T, QString>) {
                std::string str = value.toStdString();
                std::string escaped;
                escaped.reserve(str.size() + 2);
                escaped.push_back('\'');

                for (char c : str) {
                    if (c == '\'') {
                        escaped.append("''");
                    } else {
                        escaped.push_back(c);
                    }
                }

                escaped.push_back('\'');
                return escaped;
            } else if constexpr(std::is_same_v<T, QDateTime>) {
                return std::format("'{}'", value.toString(Qt::ISODate).toStdString());
#endif
            }
            return "NULL"; // Fallback for any unexpected type
        }, storage_);
    }

    [[nodiscard]] bool isNull() const { return std::holds_alternative<std::monostate>(storage_); }
};

// Condition class for SQL conditions
template<typename Config>
class ConditionStorage;

template<typename Config>
class ConditionRef {
private:
    size_t index_;
    ConditionStorage<Config>* storage_;

public:
    ConditionRef() : index_(0), storage_(nullptr) {}
    ConditionRef(size_t idx, ConditionStorage<Config>* storage)
        : index_(idx), storage_(storage) {}

    size_t index() const { return index_; }
    ConditionStorage<Config>* storage() const { return storage_; }
    bool isValid() const { return storage_ != nullptr; }

    // Used for conditional access patterns
    explicit operator bool() const { return isValid(); }
};

template<typename Config>
class ConditionStorage {
private:
    struct ConditionData {
        enum class Type { Simple, Compound };
        Type type;

        // For simple conditions
        std::string_view column;
        typename Condition<Config>::Op op;
        std::array<SqlValue<Config>, 2> values;
        size_t value_count;
        bool negated;
        std::string_view condition_str;

        // For column-to-column comparisons
        std::string_view right_column;
        bool is_column_comparison;

        // For compound conditions
        ConditionRef<Config> left;
        ConditionRef<Config> right;
    };

    // Fixed-size storage for conditions
    std::array<ConditionData, Config::MaxConditions> conditions_;
    size_t count_{0};

    static ConditionStorage<Config>& instance() {
        static ConditionStorage<Config> storage;
        return storage;
    }

public:
    static ConditionRef<Config> createSimple(std::string_view column,
                                             typename Condition<Config>::Op op,
                                             SqlValue<Config> value,
                                             bool negated = false) {
        auto& storage = instance();
        if (storage.count_ >= Config::MaxConditions) {
            return ConditionRef<Config>{};
        }

        auto& data = storage.conditions_[storage.count_];
        data.type = ConditionData::Type::Simple;
        data.column = column;
        data.op = op;
        data.values[0] = value;
        data.value_count = 1;
        data.negated = negated;
        data.is_column_comparison = false;

        return ConditionRef<Config>{storage.count_++, &storage};
    }

    // Create a column-to-column comparison
    static ConditionRef<Config> createColumnComparison(std::string_view left_col,
                                                       typename Condition<Config>::Op op,
                                                       std::string_view right_col,
                                                       bool negated = false) {
        auto& storage = instance();
        if (storage.count_ >= Config::MaxConditions) {
            return ConditionRef<Config>{};
        }

        auto& data = storage.conditions_[storage.count_];
        data.type = ConditionData::Type::Simple;
        data.column = left_col;
        data.op = op;
        data.value_count = 0;
        data.negated = negated;
        data.right_column = right_col;
        data.is_column_comparison = true;

        return ConditionRef<Config>{storage.count_++, &storage};
    }

    // Create a between condition
    static ConditionRef<Config> createBetween(std::string_view column,
                                              SqlValue<Config> start,
                                              SqlValue<Config> end,
                                              bool negated = false) {
        auto& storage = instance();
        if (storage.count_ >= Config::MaxConditions) {
            return ConditionRef<Config>{};
        }

        auto& data = storage.conditions_[storage.count_];
        data.type = ConditionData::Type::Simple;
        data.column = column;
        data.op = Condition<Config>::Op::Between;
        data.values[0] = start;
        data.values[1] = end;
        data.value_count = 2;
        data.negated = negated;
        data.is_column_comparison = false;

        return ConditionRef<Config>{storage.count_++, &storage};
    }

    // Create a raw condition
    static ConditionRef<Config> createRaw(std::string_view condition_str,
                                          bool negated = false) {
        auto& storage = instance();
        if (storage.count_ >= Config::MaxConditions) {
            return ConditionRef<Config>{};
        }

        auto& data = storage.conditions_[storage.count_];
        data.type = ConditionData::Type::Simple;
        data.column = "";
        data.op = Condition<Config>::Op::Raw;
        data.value_count = 0;
        data.negated = negated;
        data.condition_str = condition_str;
        data.is_column_comparison = false;

        return ConditionRef<Config>{storage.count_++, &storage};
    }

    // Create a compound condition (AND/OR)
    static ConditionRef<Config> createCompound(const ConditionRef<Config>& left,
                                               typename Condition<Config>::Op op,
                                               const ConditionRef<Config>& right,
                                               bool negated = false) {
        auto& storage = instance();
        if (storage.count_ >= Config::MaxConditions) {
            return ConditionRef<Config>{};
        }

        auto& data = storage.conditions_[storage.count_];
        data.type = ConditionData::Type::Compound;
        data.op = op;
        data.negated = negated;
        data.left = left;
        data.right = right;

        return ConditionRef<Config>{storage.count_++, &storage};
    }

    // Get condition data
    const ConditionData& get(size_t index) const {
        return conditions_[index];
    }

    void toString(size_t index, std::ostringstream& oss) const {
        const auto& data = conditions_[index];

        if (data.type == ConditionData::Type::Simple) {
            if (data.op == Condition<Config>::Op::Raw) {
                oss << data.condition_str;
                return;
            }

            if (data.negated) {
                oss << "NOT (";
            }

            switch (data.op) {
            case Condition<Config>::Op::IsNull:
                oss << data.column << " IS NULL";
                break;
            case Condition<Config>::Op::IsNotNull:
                oss << data.column << " IS NOT NULL";
                break;
            case Condition<Config>::Op::Between:
                if (data.value_count == 2) {
                    oss << data.column << " BETWEEN " << data.values[0].toSqlString()
                    << " AND " << data.values[1].toSqlString();
                }
                break;
            default:
                const char* op_str = "";
                switch (data.op) {
                case Condition<Config>::Op::Eq: op_str = "="; break;
                case Condition<Config>::Op::Ne: op_str = "!="; break;
                case Condition<Config>::Op::Lt: op_str = "<"; break;
                case Condition<Config>::Op::Le: op_str = "<="; break;
                case Condition<Config>::Op::Gt: op_str = ">"; break;
                case Condition<Config>::Op::Ge: op_str = ">="; break;
                case Condition<Config>::Op::Like: op_str = "LIKE"; break;
                case Condition<Config>::Op::NotLike: op_str = "NOT LIKE"; break;
                default: break;
                }

                oss << data.column << " " << op_str << " ";
                if (data.is_column_comparison) {
                    oss << data.right_column;
                } else if (data.value_count > 0) {
                    oss << data.values[0].toSqlString();
                }
            }

            if (data.negated) {
                oss << ")";
            }
        } else if (data.type == ConditionData::Type::Compound) {
            if (data.negated) oss << "NOT (";
            oss << "(";
            if (data.left.isValid()) {
                toString(data.left.index(), oss);
            } else {
                oss << "NULL";
            }
            oss << ") " << (data.op == Condition<Config>::Op::And ? "AND" : "OR") << " (";
            if (data.right.isValid()) {
                toString(data.right.index(), oss);
            } else {
                oss << "NULL";
            }
            oss << ")";
            if (data.negated) oss << ")";
        }
    }
};

// Condition class
template<typename Config>
class Condition {
public:
    enum class Op : uint8_t {
        Eq, Ne, Lt, Le, Gt, Ge,
        IsNull, IsNotNull,
        In, NotIn,
        Like, NotLike,
        Between,
        Raw,
        And,
        Or
    };

    ConditionRef<Config> ref_;

public:
    Condition() = default;

    explicit Condition(std::string_view raw_condition)
        : ref_(ConditionStorage<Config>::createRaw(raw_condition)) {}

    Condition(std::string_view col, Op op, SqlValue<Config> value)
        : ref_(ConditionStorage<Config>::createSimple(col, op, value)) {}

    Condition(std::string_view left_col, Op op, std::string_view right_col)
        : ref_(ConditionStorage<Config>::createColumnComparison(left_col, op, right_col)) {}

    Condition(std::string_view col, Op op, SqlValue<Config> value1, SqlValue<Config> value2)
        : ref_(ConditionStorage<Config>::createBetween(col, value1, value2)) {}

    Condition(const Condition& lhs, Op op, const Condition& rhs)
        : ref_(ConditionStorage<Config>::createCompound(lhs.ref_, op, rhs.ref_)) {}

    template<typename OtherConfig>
    Condition(const Condition<OtherConfig>& other) {
        if (other.ref_.isValid()) {
            std::string conditionStr = other.toString();
            ref_ = ConditionStorage<Config>::createRaw(conditionStr);
        }
    }
    // Logical operators
    Condition operator!() const {
        // TODO
        return *this; // Placeholder
    }

    Condition operator&&(const Condition& other) const {
        return Condition(*this, Op::And, other);
    }

    Condition operator||(const Condition& other) const {
        return Condition(*this, Op::Or, other);
    }

    // String conversion
    void toString(std::ostringstream& oss) const {
        if (ref_.isValid()) {
            ref_.storage()->toString(ref_.index(), oss);
        } else {
            oss << "INVALID CONDITION";
        }
    }

    [[nodiscard]] std::string toString() const {
        std::ostringstream oss;
        toString(oss);
        return oss.str();
    }
};

// TypedColumn method implementations
template<typename T, typename Config>
Condition<Config> TypedColumn<T, Config>::isNull() const {
    return Condition<Config>(name_, Condition<Config>::Op::IsNull, SqlValue<Config>());
}

template<typename T, typename Config>
Condition<Config> TypedColumn<T, Config>::isNotNull() const {
    return Condition<Config>(name_, Condition<Config>::Op::IsNotNull, SqlValue<Config>());
}

template<typename T, typename Config>
template<SqlCompatible U>
Condition<Config> TypedColumn<T, Config>::eq(U&& value) const {
    return Condition<Config>(name_, Condition<Config>::Op::Eq, SqlValue<Config>(std::forward<U>(value)));
}

template<typename T, typename Config>
template<typename OtherType, typename OtherConfig>
Condition<Config> TypedColumn<T, Config>::eq(const TypedColumn<OtherType, OtherConfig>& other) const {
    return Condition<Config>(name_, Condition<Config>::Op::Eq, other.name());
}

template<typename T, typename Config>
template<SqlCompatible U>
Condition<Config> TypedColumn<T, Config>::ne(U&& value) const {
    return Condition<Config>(name_, Condition<Config>::Op::Ne, SqlValue<Config>(std::forward<U>(value)));
}

template<typename T, typename Config>
template<typename OtherType, typename OtherConfig>
Condition<Config> TypedColumn<T, Config>::ne(const TypedColumn<OtherType, OtherConfig>& other) const {
    return Condition<Config>(name_, Condition<Config>::Op::Ne, other.name());
}

template<typename T, typename Config>
template<SqlCompatible U>
Condition<Config> TypedColumn<T, Config>::lt(U&& value) const {
    return Condition<Config>(name_, Condition<Config>::Op::Lt, SqlValue<Config>(std::forward<U>(value)));
}

template<typename T, typename Config>
template<typename OtherType, typename OtherConfig>
Condition<Config> TypedColumn<T, Config>::lt(const TypedColumn<OtherType, OtherConfig>& other) const {
    return Condition<Config>(name_, Condition<Config>::Op::Lt, other.name());
}

template<typename T, typename Config>
template<SqlCompatible U>
Condition<Config> TypedColumn<T, Config>::le(U&& value) const {
    return Condition<Config>(name_, Condition<Config>::Op::Le, SqlValue<Config>(std::forward<U>(value)));
}

template<typename T, typename Config>
template<typename OtherType, typename OtherConfig>
Condition<Config> TypedColumn<T, Config>::le(const TypedColumn<OtherType, OtherConfig>& other) const {
    return Condition<Config>(name_, Condition<Config>::Op::Le, other.name());
}

template<typename T, typename Config>
template<SqlCompatible U>
Condition<Config> TypedColumn<T, Config>::gt(U&& value) const {
    return Condition<Config>(name_, Condition<Config>::Op::Gt, SqlValue<Config>(std::forward<U>(value)));
}

template<typename T, typename Config>
template<typename OtherType, typename OtherConfig>
Condition<Config> TypedColumn<T, Config>::gt(const TypedColumn<OtherType, OtherConfig>& other) const {
    return Condition<Config>(name_, Condition<Config>::Op::Gt, other.name());
}

template<typename T, typename Config>
template<SqlCompatible U>
Condition<Config> TypedColumn<T, Config>::ge(U&& value) const {
    return Condition<Config>(name_, Condition<Config>::Op::Ge, SqlValue<Config>(std::forward<U>(value)));
}

template<typename T, typename Config>
template<typename OtherType, typename OtherConfig>
Condition<Config> TypedColumn<T, Config>::ge(const TypedColumn<OtherType, OtherConfig>& other) const {
    return Condition<Config>(name_, Condition<Config>::Op::Ge, other.name());
}

template<typename T, typename Config>
Condition<Config> TypedColumn<T, Config>::like(std::string_view pattern) const {
    return Condition<Config>(name_, Condition<Config>::Op::Like, SqlValue<Config>(pattern));
}

template<typename T, typename Config>
Condition<Config> TypedColumn<T, Config>::notLike(std::string_view pattern) const {
    return Condition<Config>(name_, Condition<Config>::Op::NotLike, SqlValue<Config>(pattern));
}

template<typename T, typename Config>
template<SqlCompatible T1, SqlCompatible T2>
Condition<Config> TypedColumn<T, Config>::between(T1&& start, T2&& end) const {
    return Condition<Config>(
        name_,
        Condition<Config>::Op::Between,
        SqlValue<Config>(std::forward<T1>(start)),
        SqlValue<Config>(std::forward<T2>(end))
        );
}

// Column class
template<typename Config>
class Column {
    std::string_view name_;

public:
    explicit Column(std::string_view name) : name_(name) {}
    [[nodiscard]] std::string_view name() const { return name_; }

    [[nodiscard]] Condition<Config> isNull() const {
        return Condition<Config>(name_, Condition<Config>::Op::IsNull, SqlValue<Config>());
    }

    [[nodiscard]] Condition<Config> isNotNull() const {
        return Condition<Config>(name_, Condition<Config>::Op::IsNotNull, SqlValue<Config>());
    }

    template<SqlCompatible T>
    [[nodiscard]] Condition<Config> eq(T&& value) const {
        return Condition<Config>(name_, Condition<Config>::Op::Eq, SqlValue<Config>(std::forward<T>(value)));
    }

    template<SqlCompatible T>
    [[nodiscard]] Condition<Config> ne(T&& value) const {
        return Condition<Config>(name_, Condition<Config>::Op::Ne, SqlValue<Config>(std::forward<T>(value)));
    }

    template<SqlCompatible T>
    [[nodiscard]] Condition<Config> lt(T&& value) const {
        return Condition<Config>(name_, Condition<Config>::Op::Lt, SqlValue<Config>(std::forward<T>(value)));
    }

    template<SqlCompatible T>
    [[nodiscard]] Condition<Config> le(T&& value) const {
        return Condition<Config>(name_, Condition<Config>::Op::Le, SqlValue<Config>(std::forward<T>(value)));
    }

    template<SqlCompatible T>
    [[nodiscard]] Condition<Config> gt(T&& value) const {
        return Condition<Config>(name_, Condition<Config>::Op::Gt, SqlValue<Config>(std::forward<T>(value)));
    }

    template<SqlCompatible T>
    [[nodiscard]] Condition<Config> ge(T&& value) const {
        return Condition<Config>(name_, Condition<Config>::Op::Ge, SqlValue<Config>(std::forward<T>(value)));
    }

    [[nodiscard]] Condition<Config> like(std::string_view pattern) const {
        return Condition<Config>(name_, Condition<Config>::Op::Like, SqlValue<Config>(pattern));
    }

    [[nodiscard]] Condition<Config> notLike(std::string_view pattern) const {
        return Condition<Config>(name_, Condition<Config>::Op::NotLike, SqlValue<Config>(pattern));
    }

    template<SqlCompatible T, SqlCompatible U>
    [[nodiscard]] Condition<Config> between(T&& start, U&& end) const {
        return Condition<Config>(
            name_,
            Condition<Config>::Op::Between,
            SqlValue<Config>(std::forward<T>(start)),
            SqlValue<Config>(std::forward<U>(end))
            );
    }
};

// Operator overloads for TypedColumn
template<typename T, typename Config, SqlCompatible U>
inline Condition<Config> operator==(const TypedColumn<T, Config>& col, U&& val) {
    return col.eq(std::forward<U>(val));
}

// Modified operator for TypedColumn == TypedColumn comparisons with different configs
template<typename T, typename U, typename ConfigT, typename ConfigU>
inline Condition<ConfigT> operator==(const TypedColumn<T, ConfigT>& col1, const TypedColumn<U, ConfigU>& col2) {
    return col1.eq(col2);
}

template<typename T, typename Config, SqlCompatible U>
inline Condition<Config> operator!=(const TypedColumn<T, Config>& col, U&& val) {
    return col.ne(std::forward<U>(val));
}

// Modified operator for TypedColumn != TypedColumn comparisons with different configs
template<typename T, typename U, typename ConfigT, typename ConfigU>
inline Condition<ConfigT> operator!=(const TypedColumn<T, ConfigT>& col1, const TypedColumn<U, ConfigU>& col2) {
    return col1.ne(col2);
}

template<typename T, typename Config, SqlCompatible U>
inline Condition<Config> operator<(const TypedColumn<T, Config>& col, U&& val) {
    return col.lt(std::forward<U>(val));
}

// Modified operator for TypedColumn < TypedColumn comparisons with different configs
template<typename T, typename U, typename ConfigT, typename ConfigU>
inline Condition<ConfigT> operator<(const TypedColumn<T, ConfigT>& col1, const TypedColumn<U, ConfigU>& col2) {
    return col1.lt(col2);
}

template<typename T, typename Config, SqlCompatible U>
inline Condition<Config> operator<=(const TypedColumn<T, Config>& col, U&& val) {
    return col.le(std::forward<U>(val));
}

// Modified operator for TypedColumn <= TypedColumn comparisons with different configs
template<typename T, typename U, typename ConfigT, typename ConfigU>
inline Condition<ConfigT> operator<=(const TypedColumn<T, ConfigT>& col1, const TypedColumn<U, ConfigU>& col2) {
    return col1.le(col2);
}

template<typename T, typename Config, SqlCompatible U>
inline Condition<Config> operator>(const TypedColumn<T, Config>& col, U&& val) {
    return col.gt(std::forward<U>(val));
}

// Modified operator for TypedColumn > TypedColumn comparisons with different configs
template<typename T, typename U, typename ConfigT, typename ConfigU>
inline Condition<ConfigT> operator>(const TypedColumn<T, ConfigT>& col1, const TypedColumn<U, ConfigU>& col2) {
    return col1.gt(col2);
}

template<typename T, typename Config, SqlCompatible U>
inline Condition<Config> operator>=(const TypedColumn<T, Config>& col, U&& val) {
    return col.ge(std::forward<U>(val));
}

// Modified operator for TypedColumn >= TypedColumn comparisons with different configs
template<typename T, typename U, typename ConfigT, typename ConfigU>
inline Condition<ConfigT> operator>=(const TypedColumn<T, ConfigT>& col1, const TypedColumn<U, ConfigU>& col2) {
    return col1.ge(col2);
}

// Operator overloads for Column
template<typename Config>
inline Condition<Config> operator==(const Column<Config>& col, const SqlValue<Config>& val) {
    return col.eq(val);
}

template<typename Config, SqlCompatible T>
inline Condition<Config> operator==(const Column<Config>& col, T&& val) {
    return col.eq(std::forward<T>(val));
}

template<typename Config>
inline Condition<Config> operator==(const Column<Config>& col, const std::string& val) {
    return col.eq(std::string_view(val));
}

template<typename Config>
inline Condition<Config> operator!=(const Column<Config>& col, const SqlValue<Config>& val) {
    return col.ne(val);
}

template<typename Config, SqlCompatible T>
inline Condition<Config> operator!=(const Column<Config>& col, T&& val) {
    return col.ne(std::forward<T>(val));
}

template<typename Config>
inline Condition<Config> operator<(const Column<Config>& col, const SqlValue<Config>& val) {
    return col.lt(val);
}

template<typename Config, SqlCompatible T>
inline Condition<Config> operator<(const Column<Config>& col, T&& val) {
    return col.lt(std::forward<T>(val));
}

template<typename Config>
inline Condition<Config> operator<=(const Column<Config>& col, const SqlValue<Config>& val) {
    return col.le(val);
}

template<typename Config, SqlCompatible T>
inline Condition<Config> operator<=(const Column<Config>& col, T&& val) {
    return col.le(std::forward<T>(val));
}

template<typename Config>
inline Condition<Config> operator>(const Column<Config>& col, const SqlValue<Config>& val) {
    return col.gt(val);
}

template<typename Config, SqlCompatible T>
inline Condition<Config> operator>(const Column<Config>& col, T&& val) {
    return col.gt(std::forward<T>(val));
}

template<typename Config>
inline Condition<Config> operator>=(const Column<Config>& col, const SqlValue<Config>& val) {
    return col.ge(val);
}

template<typename Config, SqlCompatible T>
inline Condition<Config> operator>=(const Column<Config>& col, T&& val) {
    return col.ge(std::forward<T>(val));
}

// Qt type operator overloads
#ifdef SQLQUERYBUILDER_USE_QT
template<typename Config>
inline Condition<Config> operator==(const Column<Config>& col, const QString& val) {
    return col.eq(std::string_view(val.toStdString()));
}

template<typename Config>
inline Condition<Config> operator!=(const Column<Config>& col, const QString& val) {
    return col.ne(std::string_view(val.toStdString()));
}

template<typename Config>
inline Condition<Config> operator<(const Column<Config>& col, const QDateTime& val) {
    return col.lt(val);
}

template<typename Config>
inline Condition<Config> operator<=(const Column<Config>& col, const QDateTime& val) {
    return col.le(val);
}

template<typename Config>
inline Condition<Config> operator>(const Column<Config>& col, const QDateTime& val) {
    return col.gt(val);
}

template<typename Config>
inline Condition<Config> operator>=(const Column<Config>& col, const QDateTime& val) {
    return col.ge(val);
}
inline std::ostream& operator<<(std::ostream& os, const QString& str) {
    return os << str.toStdString();
}
#endif

// Join class
template<typename Config>
class Join {
public:
    enum class Type : uint8_t { Inner, Left, Right, Full, Cross };

private:
    Type type_;
    std::string_view table_;
    std::string_view condition_;

public:
    Join() = default;

    Join(Type type, std::string_view table, std::string_view condition)
        : type_(type), table_(table), condition_(condition) {}

    void toString(std::ostringstream& oss) const {
        const char* type_str = "";
        switch (type_) {
        case Type::Inner: type_str = "INNER JOIN"; break;
        case Type::Left:  type_str = "LEFT JOIN"; break;
        case Type::Right: type_str = "RIGHT JOIN"; break;
        case Type::Full:  type_str = "FULL JOIN"; break;
        case Type::Cross: type_str = "CROSS JOIN"; break;
        }

        oss << type_str << " " << table_ << " ON " << condition_;
    }

    [[nodiscard]] std::string toString() const {
        std::ostringstream oss;
        toString(oss);
        return oss.str();
    }
};

// Main QueryBuilder class
template<typename Config = DefaultConfig>
class QueryBuilder {
public:
    enum class QueryType : uint8_t { Select, Insert, InsertOrReplace, Update, Delete, Truncate };

private:
    QueryType type_{QueryType::Select};
    std::string_view table_;

    std::array<std::string_view, Config::MaxColumns> select_columns_{};
    size_t select_columns_count_{0};

    std::array<std::pair<std::string_view, SqlValue<Config>>, Config::MaxColumns> values_{};
    size_t values_count_{0};

    std::array<Condition<Config>, Config::MaxConditions> where_conditions_{};
    size_t where_conditions_count_{0};

    std::array<Join<Config>, Config::MaxJoins> joins_{};
    size_t joins_count_{0};

    std::array<std::pair<std::string_view, bool>, Config::MaxOrderBy> order_by_{};
    size_t order_by_count_{0};

    std::array<std::string_view, Config::MaxGroupBy> group_by_{};
    size_t group_by_count_{0};

    std::string_view having_;
    int32_t limit_{-1};
    int32_t offset_{-1};
    bool distinct_{false};
    mutable std::optional<QueryError> last_error_;

    [[nodiscard]] size_t estimateSize() const {
        size_t size = 64; // Base size
        size += table_.size();
        size += select_columns_count_ * 10;
        size += where_conditions_count_ * 30;
        size += joins_count_ * 40;
        size += order_by_count_ * 15;
        size += group_by_count_ * 15;
        size += having_.size();
        if (limit_ >= 0) size += 15;
        if (offset_ >= 0) size += 15;
        return size;
    }

public:
    QueryBuilder() = default;

    QueryBuilder& reset() {
        type_ = QueryType::Select;
        table_ = "";
        select_columns_count_ = 0;
        values_count_ = 0;
        where_conditions_count_ = 0;
        joins_count_ = 0;
        order_by_count_ = 0;
        group_by_count_ = 0;
        having_ = "";
        limit_ = -1;
        offset_ = -1;
        distinct_ = false;
        last_error_ = std::nullopt;
        return *this;
    }

    [[nodiscard]] std::optional<QueryError> lastError() const {
        return last_error_;
    }

    template<typename... Cols>
    QueryBuilder& select(Cols&&... cols) {
        type_ = QueryType::Select;
        const size_t additional = sizeof...(cols);

        if (select_columns_count_ + additional > Config::MaxColumns) {
            auto error = QueryError(QueryError::Code::TooManyColumns,
                                    std::format("Too many columns: limit is {}", Config::MaxColumns));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        (addSelectColumn(std::forward<Cols>(cols)), ...);
        return *this;
    }

    template<typename T, size_t Extent>
    QueryBuilder& select(std::span<const T, Extent> cols) {
        type_ = QueryType::Select;

        if (select_columns_count_ + cols.size() > Config::MaxColumns) {
            auto error = QueryError(QueryError::Code::TooManyColumns,
                                    std::format("Too many columns: limit is {}", Config::MaxColumns));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        for (const auto& col : cols) {
            if constexpr (std::is_convertible_v<T, std::string_view>) {
                select_columns_[select_columns_count_++] = static_cast<std::string_view>(col);
            }
        }
        return *this;
    }

    template<typename T>
    void addSelectColumn(const T& col) {
        if constexpr (std::is_convertible_v<T, std::string_view>) {
            select_columns_[select_columns_count_++] = static_cast<std::string_view>(col);
        } else {
            static_assert(std::is_convertible_v<T, std::string_view>,
                          "Column type not supported for select");
        }
    }

    // Select from span
    QueryBuilder& select(std::span<const std::string_view> cols) {
        type_ = QueryType::Select;

        if (select_columns_count_ + cols.size() > Config::MaxColumns) {
            auto error = QueryError(QueryError::Code::TooManyColumns,
                                    std::format("Too many columns: limit is {}", Config::MaxColumns));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        for (const auto& col : cols) {
            select_columns_[select_columns_count_++] = col;
        }
        return *this;
    }

    // Select from array
    template<size_t N>
    QueryBuilder& select(const std::array<std::string_view, N>& cols) {
        return select(std::span<const std::string_view>(cols));
    }

    // From method
    template<typename T>
    QueryBuilder& from(const T& table) {
        if constexpr (std::is_convertible_v<T, std::string_view>) {
            table_ = static_cast<std::string_view>(table);
        } else {
            static_assert(std::is_convertible_v<T, std::string_view>,
                          "Table type not supported for from");
        }
        return *this;
    }

    template<typename OtherConfig>
    QueryBuilder& where(const Condition<OtherConfig>& condition) {
        if (where_conditions_count_ >= Config::MaxConditions) {
            auto error = QueryError(QueryError::Code::TooManyConditions,
                                    std::format("Too many conditions: limit is {}", Config::MaxConditions));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        // Convert condition from OtherConfig to our Config
        where_conditions_[where_conditions_count_++] = Condition<Config>(condition);
        return *this;
    }

    template<SqlCompatible T>
    QueryBuilder& whereOp(std::string_view column, typename Condition<Config>::Op op, T&& value) {
        if (where_conditions_count_ >= Config::MaxConditions) {
            auto error = QueryError(QueryError::Code::TooManyConditions,
                                    std::format("Too many conditions: limit is {}", Config::MaxConditions));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        where_conditions_[where_conditions_count_++] = Condition<Config>(
            column, op, SqlValue<Config>(std::forward<T>(value))
            );
        return *this;
    }

    // OrderBy with bounds checking
    template<typename T>
    QueryBuilder& orderBy(const T& column, bool asc = true) {
        if (order_by_count_ >= Config::MaxOrderBy) {
            auto error = QueryError(QueryError::Code::TooManyOrderBy,
                                    std::format("Too many order by clauses: limit is {}", Config::MaxOrderBy));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        if constexpr (std::is_convertible_v<T, std::string_view>) {
            order_by_[order_by_count_++] = {static_cast<std::string_view>(column), asc};
        } else {
            static_assert(std::is_convertible_v<T, std::string_view>,
                          "Column type not supported for orderBy");
        }
        return *this;
    }

    QueryBuilder& limit(int32_t limit) {
        limit_ = limit;
        return *this;
    }

    QueryBuilder& offset(int32_t offset) {
        offset_ = offset;
        return *this;
    }

    QueryBuilder& distinct() {
        distinct_ = true;
        return *this;
    }

    // Insert with bounds checking
    template<typename T>
    QueryBuilder& insert(const T& table) {
        type_ = QueryType::Insert;
        if constexpr (std::is_convertible_v<T, std::string_view>) {
            table_ = static_cast<std::string_view>(table);
        } else {
            static_assert(std::is_convertible_v<T, std::string_view>,
                          "Table type not supported for insert");
        }
        return *this;
    }

    // Insert OR REPLACE with bounds checking
    template<typename T>
    QueryBuilder& insertOrReplace(const T& table) {
        type_ = QueryType::InsertOrReplace;
        if constexpr (std::is_convertible_v<T, std::string_view>) {
            table_ = static_cast<std::string_view>(table);
        } else {
            static_assert(std::is_convertible_v<T, std::string_view>,
                          "Table type not supported for insertOrReplace");
        }
        return *this;
    }

    template<typename Col, SqlCompatible T>
    QueryBuilder& value(const Col& column, T&& val) {
        if (values_count_ >= Config::MaxColumns) {
            auto error = QueryError(QueryError::Code::TooManyColumns,
                                    std::format("Too many values: limit is {}", Config::MaxColumns));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        if constexpr (std::is_convertible_v<Col, std::string_view>) {
            // Direct assignment of the pair at the current index
            values_[values_count_].first = static_cast<std::string_view>(column);
            values_[values_count_].second = SqlValue<Config>(std::forward<T>(val));
            values_count_++;
        } else {
            static_assert(std::is_convertible_v<Col, std::string_view>,
                          "Column type not supported for value");
        }
        return *this;
    }

    // Update methods
    template<typename T>
    QueryBuilder& update(const T& table) {
        type_ = QueryType::Update;
        if constexpr (std::is_convertible_v<T, std::string_view>) {
            table_ = static_cast<std::string_view>(table);
        } else {
            static_assert(std::is_convertible_v<T, std::string_view>,
                          "Table type not supported for update");
        }
        return *this;
    }

    template<typename Col, SqlCompatible T>
    QueryBuilder& set(const Col& column, T&& val) {
        if (values_count_ >= Config::MaxColumns) {
            auto error = QueryError(QueryError::Code::TooManyColumns,
                                    std::format("Too many values: limit is {}", Config::MaxColumns));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        if constexpr (std::is_convertible_v<Col, std::string_view>) {
            // Direct assignment of the pair at the current index
            values_[values_count_].first = static_cast<std::string_view>(column);
            values_[values_count_].second = SqlValue<Config>(std::forward<T>(val));
            values_count_++;
        } else {
            static_assert(std::is_convertible_v<Col, std::string_view>,
                          "Column type not supported for set");
        }
        return *this;
    }

    // Delete methods
    template<typename T>
    QueryBuilder& deleteFrom(const T& table) {
        type_ = QueryType::Delete;
        if constexpr (std::is_convertible_v<T, std::string_view>) {
            table_ = static_cast<std::string_view>(table);
        } else {
            static_assert(std::is_convertible_v<T, std::string_view>,
                          "Table type not supported for deleteFrom");
        }
        return *this;
    }

    // Truncate methods
    template<typename T>
    QueryBuilder& truncate(const T& table) {
        type_ = QueryType::Truncate;
        if constexpr (std::is_convertible_v<T, std::string_view>) {
            table_ = static_cast<std::string_view>(table);
        } else {
            static_assert(std::is_convertible_v<T, std::string_view>,
                          "Table type not supported for truncate");
        }
        return *this;
    }

    // Join methods with bounds checking
    template<typename T>
    QueryBuilder& innerJoin(const T& table, std::string_view condition) {
        if (joins_count_ >= Config::MaxJoins) {
            auto error = QueryError(QueryError::Code::TooManyJoins,
                                    std::format("Too many joins: limit is {}", Config::MaxJoins));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        if constexpr (std::is_convertible_v<T, std::string_view>) {
            joins_[joins_count_++] = Join<Config>{
                Join<Config>::Type::Inner,
                static_cast<std::string_view>(table),
                condition
            };
        } else {
            static_assert(std::is_convertible_v<T, std::string_view>,
                          "Table type not supported for innerJoin");
        }
        return *this;
    }

    template<typename T>
    QueryBuilder& leftJoin(const T& table, std::string_view condition) {
        if (joins_count_ >= Config::MaxJoins) {
            auto error = QueryError(QueryError::Code::TooManyJoins,
                                    std::format("Too many joins: limit is {}", Config::MaxJoins));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        if constexpr (std::is_convertible_v<T, std::string_view>) {
            joins_[joins_count_++] = Join<Config>{
                Join<Config>::Type::Left,
                static_cast<std::string_view>(table),
                condition
            };
        } else {
            static_assert(std::is_convertible_v<T, std::string_view>,
                          "Table type not supported for leftJoin");
        }
        return *this;
    }

    template<typename T>
    QueryBuilder& rightJoin(const T& table, std::string_view condition) {
        if (joins_count_ >= Config::MaxJoins) {
            auto error = QueryError(QueryError::Code::TooManyJoins,
                                    std::format("Too many joins: limit is {}", Config::MaxJoins));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        if constexpr (std::is_convertible_v<T, std::string_view>) {
            joins_[joins_count_++] = Join<Config>{
                Join<Config>::Type::Right,
                static_cast<std::string_view>(table),
                condition
            };
        } else {
            static_assert(std::is_convertible_v<T, std::string_view>,
                          "Table type not supported for rightJoin");
        }
        return *this;
    }

    template<typename T>
    QueryBuilder& fullJoin(const T& table, std::string_view condition) {
        if (joins_count_ >= Config::MaxJoins) {
            auto error = QueryError(QueryError::Code::TooManyJoins,
                                    std::format("Too many joins: limit is {}", Config::MaxJoins));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        if constexpr (std::is_convertible_v<T, std::string_view>) {
            joins_[joins_count_++] = Join<Config>{
                Join<Config>::Type::Full,
                static_cast<std::string_view>(table),
                condition
            };
        } else {
            static_assert(std::is_convertible_v<T, std::string_view>,
                          "Table type not supported for fullJoin");
        }
        return *this;
    }

    // Where variations
    template<typename Col, typename T>
    QueryBuilder& whereIn(const Col& column, std::span<const T> values) {
        if (where_conditions_count_ >= Config::MaxConditions) {
            auto error = QueryError(QueryError::Code::TooManyConditions,
                                    std::format("Too many conditions: limit is {}", Config::MaxConditions));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        std::ostringstream condition;
        if constexpr (std::is_convertible_v<Col, std::string_view>) {
            condition << static_cast<std::string_view>(column) << " IN (";
            for (size_t i = 0; i < values.size(); ++i) {
                if (i > 0) condition << ", ";
                condition << SqlValue<Config>(values[i]).toSqlString();
            }
            condition << ")";
            where_conditions_[where_conditions_count_++] = Condition<Config>(condition.str());
        } else {
            static_assert(std::is_convertible_v<Col, std::string_view>,
                          "Column type not supported for whereIn");
        }
        return *this;
    }

    template<typename Col, typename T, typename U>
    QueryBuilder& whereBetween(const Col& column, T&& start, U&& end) {
        if (where_conditions_count_ >= Config::MaxConditions) {
            auto error = QueryError(QueryError::Code::TooManyConditions,
                                    std::format("Too many conditions: limit is {}", Config::MaxConditions));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        if constexpr (std::is_convertible_v<Col, std::string_view>) {
            where_conditions_[where_conditions_count_++] = Condition<Config>(
                static_cast<std::string_view>(column),
                Condition<Config>::Op::Between,
                SqlValue<Config>(std::forward<T>(start)),
                SqlValue<Config>(std::forward<U>(end))
                );
        } else {
            static_assert(std::is_convertible_v<Col, std::string_view>,
                          "Column type not supported for whereBetween");
        }
        return *this;
    }

    template<typename Col>
    QueryBuilder& whereLike(const Col& column, std::string_view pattern) {
        if (where_conditions_count_ >= Config::MaxConditions) {
            auto error = QueryError(QueryError::Code::TooManyConditions,
                                    std::format("Too many conditions: limit is {}", Config::MaxConditions));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        if constexpr (std::is_convertible_v<Col, std::string_view>) {
            where_conditions_[where_conditions_count_++] = Condition<Config>(
                static_cast<std::string_view>(column),
                Condition<Config>::Op::Like,
                SqlValue<Config>(pattern)
                );
        } else {
            static_assert(std::is_convertible_v<Col, std::string_view>,
                          "Column type not supported for whereLike");
        }
        return *this;
    }

    template<typename Col>
    QueryBuilder& whereNull(const Col& column) {
        if (where_conditions_count_ >= Config::MaxConditions) {
            auto error = QueryError(QueryError::Code::TooManyConditions,
                                    std::format("Too many conditions: limit is {}", Config::MaxConditions));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        if constexpr (std::is_convertible_v<Col, std::string_view>) {
            where_conditions_[where_conditions_count_++] = Condition<Config>(
                static_cast<std::string_view>(column),
                Condition<Config>::Op::IsNull,
                SqlValue<Config>()
                );
        } else {
            static_assert(std::is_convertible_v<Col, std::string_view>,
                          "Column type not supported for whereNull");
        }
        return *this;
    }

    template<typename Col>
    QueryBuilder& whereNotNull(const Col& column) {
        if (where_conditions_count_ >= Config::MaxConditions) {
            auto error = QueryError(QueryError::Code::TooManyConditions,
                                    std::format("Too many conditions: limit is {}", Config::MaxConditions));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        if constexpr (std::is_convertible_v<Col, std::string_view>) {
            where_conditions_[where_conditions_count_++] = Condition<Config>(
                static_cast<std::string_view>(column),
                Condition<Config>::Op::IsNotNull,
                SqlValue<Config>()
                );
        } else {
            static_assert(std::is_convertible_v<Col, std::string_view>,
                          "Column type not supported for whereNotNull");
        }
        return *this;
    }

    QueryBuilder& whereExists(std::string_view subquery) {
        if (where_conditions_count_ >= Config::MaxConditions) {
            auto error = QueryError(QueryError::Code::TooManyConditions,
                                    std::format("Too many conditions: limit is {}", Config::MaxConditions));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        std::string condition = std::string("EXISTS (") + std::string(subquery) + ")";
        where_conditions_[where_conditions_count_++] = Condition<Config>(condition);
        return *this;
    }

    QueryBuilder& whereRaw(std::string_view rawCondition) {
        if (where_conditions_count_ >= Config::MaxConditions) {
            auto error = QueryError(QueryError::Code::TooManyConditions,
                                    std::format("Too many conditions: limit is {}", Config::MaxConditions));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        where_conditions_[where_conditions_count_++] = Condition<Config>(rawCondition);
        return *this;
    }

    template<typename Col>
    QueryBuilder& groupBy(const Col& column) {
        if (group_by_count_ >= Config::MaxGroupBy) {
            auto error = QueryError(QueryError::Code::TooManyGroupBy,
                                    std::format("Too many group by clauses: limit is {}", Config::MaxGroupBy));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        if constexpr (std::is_convertible_v<Col, std::string_view>) {
            group_by_[group_by_count_++] = static_cast<std::string_view>(column);
        } else {
            static_assert(std::is_convertible_v<Col, std::string_view>,
                          "Column type not supported for groupBy");
        }
        return *this;
    }

    QueryBuilder& having(std::string_view condition) {
        having_ = condition;
        return *this;
    }

    // Build with error handling
    [[nodiscard]] Result<std::string> buildResult() const {
        if (table_.empty()) {
            QueryError error(QueryError::Code::EmptyTable, "Table name is required");
            last_error_ = error;
            return error;
        }

        try {
            std::string query;
            query.reserve(estimateSize());

            switch (type_) {
            case QueryType::Select:
                buildSelect(query);
                break;
            case QueryType::Insert:
                buildInsert(query, false);
                break;
            case QueryType::InsertOrReplace:
                buildInsert(query, true);
                break;
            case QueryType::Update:
                buildUpdate(query);
                break;
            case QueryType::Delete:
                buildDelete(query);
                break;
            case QueryType::Truncate:
                buildTruncate(query);
                break;
            }

            return Result<std::string>(query);
        } catch (const std::exception& e) {
            QueryError error(QueryError::Code::InvalidCondition, e.what());
            last_error_ = error;
            return error;
        }
    }

#ifdef SQLQUERYBUILDER_USE_QT
    [[nodiscard]] QString build() const {
        auto result = buildResult();
        if (result.hasError()) {
            if constexpr(Config::ThrowOnError) {
                throw result.error();
            }
            return QString("/* ERROR: %1 */").arg(QString::fromUtf8(result.error().message.data(),
                                                                    static_cast<int>(result.error().message.size())));
        }
        return QString::fromStdString(result.value());
    }
#else
    [[nodiscard]] std::string build() const {
        auto result = buildResult();
        if (result.hasError()) {
            if constexpr(Config::ThrowOnError) {
                throw result.error();
            }
            return "/* ERROR: " + std::string(result.error().message) + " */";
        }
        return result.value();
    }
#endif

private:
    void buildSelect(std::string& query) const {
        std::ostringstream oss;
        oss << "SELECT ";
        if (distinct_) oss << "DISTINCT ";

        if (select_columns_count_ == 0) {
            oss << "*";
        } else {
            bool first = true;
            for (size_t i = 0; i < select_columns_count_; ++i) {
                if (!first) oss << ", ";
                oss << select_columns_[i];
                first = false;
            }
        }

        oss << " FROM " << table_;

        // Joins
        for (size_t i = 0; i < joins_count_; ++i) {
            oss << " ";
            joins_[i].toString(oss);
        }

        // Where
        if (where_conditions_count_ > 0) {
            oss << " WHERE ";
            bool first = true;
            for (size_t i = 0; i < where_conditions_count_; ++i) {
                if (!first) oss << " AND ";
                where_conditions_[i].toString(oss);
                first = false;
            }
        }

        // Group By
        if (group_by_count_ > 0) {
            oss << " GROUP BY ";
            bool first = true;
            for (size_t i = 0; i < group_by_count_; ++i) {
                if (!first) oss << ", ";
                oss << group_by_[i];
                first = false;
            }

            if (!having_.empty()) {
                oss << " HAVING " << having_;
            }
        }

        // Order By
        if (order_by_count_ > 0) {
            oss << " ORDER BY ";
            bool first = true;
            for (size_t i = 0; i < order_by_count_; ++i) {
                if (!first) oss << ", ";
                oss << order_by_[i].first << (order_by_[i].second ? " ASC" : " DESC");
                first = false;
            }
        }

        if (limit_ >= 0) {
            oss << " LIMIT " << limit_;
        }

        if (offset_ >= 0) {
            oss << " OFFSET " << offset_;
        }

        query = oss.str();
    }

    void buildInsert(std::string& query, bool orReplace) const {
        if (values_count_ == 0) {
            throw QueryError(QueryError::Code::InvalidCondition, "No values specified for INSERT");
        }

        std::ostringstream oss;
        oss << "INSERT";

        if (orReplace) {
            oss << " OR REPLACE";
        }

        oss << " INTO " << table_ << " (";

        bool first = true;
        for (size_t i = 0; i < values_count_; ++i) {
            if (!first) oss << ", ";
            oss << values_[i].first;
            first = false;
        }

        oss << ") VALUES (";

        first = true;
        for (size_t i = 0; i < values_count_; ++i) {
            if (!first) oss << ", ";
            oss << values_[i].second.toSqlString();
            first = false;
        }

        oss << ")";
        query = oss.str();
    }

    void buildUpdate(std::string& query) const {
        if (values_count_ == 0) {
            throw QueryError(QueryError::Code::InvalidCondition, "No values specified for UPDATE");
        }

        std::ostringstream oss;
        oss << "UPDATE " << table_ << " SET ";

        bool first = true;
        for (size_t i = 0; i < values_count_; ++i) {
            if (!first) oss << ", ";
            oss << values_[i].first << " = " << values_[i].second.toSqlString();
            first = false;
        }

        if (where_conditions_count_ > 0) {
            oss << " WHERE ";
            first = true;
            for (size_t i = 0; i < where_conditions_count_; ++i) {
                if (!first) oss << " AND ";
                where_conditions_[i].toString(oss);
                first = false;
            }
        }

        query = oss.str();
    }

    void buildDelete(std::string& query) const {
        std::ostringstream oss;
        oss << "DELETE FROM " << table_;

        if (where_conditions_count_ > 0) {
            oss << " WHERE ";
            bool first = true;
            for (size_t i = 0; i < where_conditions_count_; ++i) {
                if (!first) oss << " AND ";
                where_conditions_[i].toString(oss);
                first = false;
            }
        }

        query = oss.str();
    }

    void buildTruncate(std::string& query) const {
        query = "TRUNCATE TABLE " + std::string(table_);
    }
};

//=====================
// SQL Aggregate Functions
//=====================

// Function to create SQL aggregate function strings
// We return std::string here because these are typically used as temporary values
// in select() calls, so we can't use string_view which would be referring to
// temporary data.

inline std::string count(std::string_view column) {
    return std::string("COUNT(") + std::string(column) + ")";
}

inline std::string countDistinct(std::string_view column) {
    return std::string("COUNT(DISTINCT ") + std::string(column) + ")";
}

inline std::string sum(std::string_view column) {
    return std::string("SUM(") + std::string(column) + ")";
}

inline std::string avg(std::string_view column) {
    return std::string("AVG(") + std::string(column) + ")";
}

inline std::string min(std::string_view column) {
    return std::string("MIN(") + std::string(column) + ")";
}

inline std::string max(std::string_view column) {
    return std::string("MAX(") + std::string(column) + ")";
}

inline std::string groupConcat(std::string_view column) {
    return std::string("GROUP_CONCAT(") + std::string(column) + ")";
}

// Function to create an alias
inline std::string as(std::string_view expr, std::string_view alias) {
    return std::string(expr) + " AS " + std::string(alias);
}

// Helper functions
template<typename Config = DefaultConfig>
[[nodiscard]] constexpr Table<Config> table(std::string_view name) {
    return Table<Config>(name);
}

template<typename T, typename Config = DefaultConfig>
[[nodiscard]] constexpr TypedColumn<T, Config> column(const Table<Config>& table, std::string_view name) {
    return TypedColumn<T, Config>(table.name(), name);
}

template<typename Config = DefaultConfig>
inline Column<Config> col(std::string_view name) {
    return Column<Config>(name);
}

template<SqlCompatible T, typename Config = DefaultConfig>
inline SqlValue<Config> val(T&& value) {
    return SqlValue<Config>(std::forward<T>(value));
}

// All columns helper
template<typename Config = DefaultConfig>
inline std::string_view all_of(const Table<Config>&) {
    return "*";
}

// Raw condition helper
template<typename Config = DefaultConfig>
inline Condition<Config> raw(std::string_view condition) {
    return Condition<Config>(condition);
}

// Macros for easy table definition
#define SQL_DEFINE_TABLE(name) \
struct name##_table { \
        const sql::Table<> table = sql::table(#name);

#define SQL_DEFINE_COLUMN(name, type) \
    const sql::TypedColumn<type> name = sql::column<type>(table, #name);

#define SQL_END_TABLE() };

} // namespace v1
} // namespace sql
