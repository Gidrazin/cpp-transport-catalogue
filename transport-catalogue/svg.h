#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include <unordered_map>

namespace svg {

using namespace std::literals;

struct Rgb {
    Rgb() = default;
    Rgb(uint8_t my_red, uint8_t my_green, uint8_t my_blue);

    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
};

struct Rgba {
    Rgba() = default;
    Rgba(uint8_t red, uint8_t green, uint8_t blue, double alpha);

    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    double opacity = 1.0;
};

using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;

inline const Color NoneColor{ std::monostate() };

std::ostream& operator<<(std::ostream& os, const Color& color);

struct Point {
    Point() = default;
    Point(double my_x, double my_y)
        : x(my_x)
        , y(my_y) {
    }
    double x = 0;
    double y = 0;
};

enum class StrokeLineCap {
    BUTT,
    ROUND,
    SQUARE,
};

enum class StrokeLineJoin {
    ARCS,
    BEVEL,
    MITER,
    MITER_CLIP,
    ROUND,
};

std::ostream& operator<<(std::ostream& os, const StrokeLineCap& cap);

std::ostream& operator<<(std::ostream& os, const StrokeLineJoin& join);

/*
 * Вспомогательная структура, хранящая контекст для вывода SVG-документа с отступами.
 * Хранит ссылку на поток вывода, текущее значение и шаг отступа при выводе элемента
 */
struct RenderContext {
    RenderContext(std::ostream& out)
        : out(out) {
    }

    RenderContext(std::ostream& out, int indent_step, int indent = 0)
        : out(out)
        , indent_step(indent_step)
        , indent(indent) {
    }

    RenderContext Indented() const {
        return {out, indent_step, indent + indent_step};
    }

    void RenderIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    std::ostream& out;
    int indent_step = 0;
    int indent = 0;
};

/*
 * Абстрактный базовый класс Object служит для унифицированного хранения
 * конкретных тегов SVG-документа
 * Реализует паттерн "Шаблонный метод" для вывода содержимого тега
 */
class Object {
public:
    void Render(const RenderContext& context) const;

    virtual ~Object() = default;

private:
    virtual void RenderObject(const RenderContext& context) const = 0;
};

class ObjectContainer {
public:
    template <typename Obj>
    void Add(Obj obj){
        AddPtr(std::make_unique<Obj>(std::move(obj)));
    }

    virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;

    virtual ~ObjectContainer() = default;
};

class Drawable {
public:
    virtual void Draw(ObjectContainer& oc) const = 0;
    virtual ~Drawable() = default;
}; 

struct ColorPrinter {
    std::ostream& ost_;
    void operator()(std::monostate) const;
    void operator()(const std::string& color) const;
    void operator()(const Rgb& color) const;
    void operator()(const Rgba& color) const;
};

template <typename Owner>
class PathProps {
public:
    Owner& SetFillColor(Color color) {
        fill_color_ = std::move(color);
        return AsOwner();
    }
    Owner& SetStrokeColor(Color color) {
        stroke_color_ = std::move(color);
        return AsOwner();
    }

    Owner& SetStrokeWidth(double width){
        stroke_width_ = width;
        return AsOwner();
    }

    Owner& SetStrokeLineCap(StrokeLineCap line_cap){
        stroke_line_cap_ = line_cap;
        return AsOwner();
    }

    Owner& SetStrokeLineJoin(StrokeLineJoin line_join){
        stroke_line_join_ = line_join;
        return AsOwner();
    }

protected:
    ~PathProps() = default;

    // Метод RenderAttrs выводит в поток общие для всех путей атрибуты
    void RenderAttrs(std::ostream& out) const {
        using namespace std::literals;

        if (fill_color_){
            out << " fill=\""sv << *fill_color_ << "\""sv;
        }
        if (stroke_color_){
            out << " stroke=\""sv << *stroke_color_ << "\""sv;
        }
        if (stroke_width_) {
            out << " stroke-width=\""sv << stroke_width_ << "\""sv;
        }
        if (stroke_line_cap_) {
            out << " stroke-linecap=\""sv << *stroke_line_cap_ << "\""sv;
        }
        if (stroke_line_join_) {
            out << " stroke-linejoin=\""sv << *stroke_line_join_ << "\""sv;
        }
    }

private:
    Owner& AsOwner() {
        // static_cast безопасно преобразует *this к Owner&,
        // если класс Owner — наследник PathProps
        return static_cast<Owner&>(*this);
    }

    std::optional<Color> fill_color_;
    std::optional<Color> stroke_color_;
    double stroke_width_ = 0;
    std::optional<StrokeLineCap> stroke_line_cap_;
    std::optional<StrokeLineJoin> stroke_line_join_;
};

/*
 * Класс Circle моделирует элемент <circle> для отображения круга
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
 */
class Circle final : public Object, public PathProps<Circle>{
public:
    Circle& SetCenter(Point center);
    Circle& SetRadius(double radius);

private:
    void RenderObject(const RenderContext& context) const override;

    Point center_ = {0.0, 0.0};
    double radius_ = 1.0;
};

/*
 * Класс Polyline моделирует элемент <polyline> для отображения ломаных линий
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
 */
class Polyline final : public Object, public PathProps<Polyline> {
public:
    // Добавляет очередную вершину к ломаной линии
    Polyline& AddPoint(Point point);

private:
    void RenderObject(const RenderContext& context) const override;

    std::vector<Point> points_;
};

/*
 * Класс Text моделирует элемент <text> для отображения текста
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
 */
class Text final : public Object, public PathProps<Text> {

public:
    // Задаёт координаты опорной точки (атрибуты x и y)
    Text& SetPosition(Point pos);

    // Задаёт смещение относительно опорной точки (атрибуты dx, dy)
    Text& SetOffset(Point offset);

    // Задаёт размеры шрифта (атрибут font-size)
    Text& SetFontSize(uint32_t size);

    // Задаёт название шрифта (атрибут font-family)
    Text& SetFontFamily(std::string font_family);

    // Задаёт толщину шрифта (атрибут font-weight)
    Text& SetFontWeight(std::string font_weight);

    // Задаёт текстовое содержимое объекта (отображается внутри тега text)
    Text& SetData(std::string data);

    // Прочие данные и методы, необходимые для реализации элемента <text>
private:
    void RenderObject(const RenderContext& context) const override;

    Point pos_ = {0.0, 0.0};
    Point offset_ = {0.0, 0.0};
    uint32_t font_size_ = 1;
    std::string font_family_;
    std::string font_weight_;
    std::string data_;
    std::unordered_map<char, std::string> map_ = {
    {'"', "&quot;"s},
    {'<', "&lt;"s},
    {'>', "&gt;"s},
    {'\'', "&apos;"s},
    {'&', "&amp;"s}};
};



class Document : public ObjectContainer{
public:
    void AddPtr(std::unique_ptr<Object>&& obj) override;

    void Render(std::ostream& out) const;
private:
    std::vector<std::unique_ptr<Object>> objects_;
};

}  // namespace svg

//-----------------SHAPES----------------------

namespace shapes {

class Triangle : public svg::Drawable {
public:
    Triangle(svg::Point p1, svg::Point p2, svg::Point p3);

    void Draw(svg::ObjectContainer& container) const override;

private:
    svg::Point p1_, p2_, p3_;
};

class Star : public svg::Drawable {
public:
    Star(svg::Point center, double outer_rad, double inner_rad, int num_rays);

    void Draw(svg::ObjectContainer& container) const override;

private:
    svg::Polyline star_;
};


class Snowman : public svg::Drawable {
public:
    Snowman(svg::Point head_center, double radius);

    void Draw(svg::ObjectContainer& container) const override;

private:
    svg::Point head_center_ = {0.0, 0.0};
    double radius_ = 1;
};

} // namespace shapes