#define _USE_MATH_DEFINES 
#include <cmath>

#include "svg.h"

namespace {
svg::Polyline CreateStar(svg::Point center, double outer_rad, double inner_rad, int num_rays) {
    using namespace svg;
    Polyline polyline;
    for (int i = 0; i <= num_rays; ++i) {
        double angle = 2 * M_PI * (i % num_rays) / num_rays;
        polyline.AddPoint({center.x + outer_rad * sin(angle), center.y - outer_rad * cos(angle)});
        if (i == num_rays) {
            break;
        }
        angle += M_PI / num_rays;
        polyline.AddPoint({center.x + inner_rad * sin(angle), center.y - inner_rad * cos(angle)});
    }
    return polyline.SetFillColor("red").SetStrokeColor("black");
}
}


namespace svg {

using namespace std::literals;

Rgb::Rgb(uint8_t my_red, uint8_t my_green, uint8_t my_blue)
: red(my_red)
, green(my_green)
, blue(my_blue)
{}

Rgba::Rgba(uint8_t red, uint8_t green, uint8_t blue, double alpha)
: red(red)
, green(green)
, blue(blue)
, opacity(alpha)
{}

std::ostream& operator<<(std::ostream& os, const StrokeLineCap& cap) {
    switch (cap) {
        case svg::StrokeLineCap::BUTT:
            os << "butt";
            break;
        case svg::StrokeLineCap::ROUND:
            os << "round";
            break;
        case svg::StrokeLineCap::SQUARE:
            os << "square";
            break;
        default:
            break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const StrokeLineJoin& join) {
    switch (join) {
        case svg::StrokeLineJoin::ARCS:
            os << "arcs";
            break;
        case svg::StrokeLineJoin::BEVEL:
            os << "bevel";
            break;
        case svg::StrokeLineJoin::MITER:
            os << "miter";
            break;
        case svg::StrokeLineJoin::MITER_CLIP:
            os << "miter-clip";
            break;
        case svg::StrokeLineJoin::ROUND:
            os << "round";
            break;
        default:
            break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const Color& color){
    std::visit(ColorPrinter{os}, color);
    return os;
}

void Object::Render(const RenderContext& context) const {
    context.RenderIndent();

    // Делегируем вывод тега своим подклассам
    RenderObject(context);

    context.out << std::endl;
}


void ColorPrinter::operator()(std::monostate) const {
    ost_ << "none"sv;
}

void ColorPrinter::operator()(const std::string& color) const {
    ost_ << color;
}

void ColorPrinter::operator()(const Rgb& color) const {
    ost_ << "rgb("s << static_cast<int>(color.red) << ","s << static_cast<int>(color.green) << ","s << static_cast<int>(color.blue) << ")"s;
}

void ColorPrinter::operator()(const Rgba& color) const {
    ost_ << "rgba("s << static_cast<int>(color.red) << ","s << static_cast<int>(color.green) << ","s << static_cast<int>(color.blue) << ","s << color.opacity << ")"s;
}


// ---------- Circle ------------------

Circle& Circle::SetCenter(Point center)  {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius)  {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
    out << "r=\""sv << radius_ << "\" "sv;
    RenderAttrs(context.out);
    out << "/>"sv;
}

// ---------- Polyline ------------------

Polyline& Polyline::AddPoint(Point point){
    points_.push_back(point);
    return *this;
}

void Polyline::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<polyline points=\""sv;
    for (size_t i = 0; i < points_.size(); ++i){
        if (i == points_.size() - 1){
            out << points_[i].x << ',' << points_[i].y;
        } else {
            out << points_[i].x << ',' << points_[i].y << ' ';
        }
        
    }
    out << "\""sv;
    RenderAttrs(out);
    out << " />"sv;
}

// ---------- Text ------------------

Text& Text::SetPosition(Point pos){
    pos_ = pos;
    return *this;
}

// Задаёт смещение относительно опорной точки (атрибуты dx, dy)
Text& Text::SetOffset(Point offset){
    offset_ = offset;
    return *this;
}
// Задаёт размеры шрифта (атрибут font-size)
Text& Text::SetFontSize(uint32_t size){
    font_size_ = size;
    return *this;
}
// Задаёт название шрифта (атрибут font-family)
Text& Text::SetFontFamily(std::string font_family){
    font_family_ = std::move(font_family);
    return *this;
}
// Задаёт толщину шрифта (атрибут font-weight)
Text& Text::SetFontWeight(std::string font_weight){
    font_weight_ = std::move(font_weight);
    return *this;
}
// Задаёт текстовое содержимое объекта (отображается внутри тега text)
Text& Text::SetData(std::string data){
    std::string result;
    for (char c : data){
        if (c == '"' || c == '<' || c == '>' || c == '\'' || c == '&'){
            result += map_[c];
            continue;
        }
        result += c;
    }
    data_ = std::move(result);
    return *this;
}
// Прочие данные и методы, необходимые для реализации элемента <text>

void Text::RenderObject(const RenderContext& context) const{
    auto& out = context.out;
    out << "<text "sv;
    out << "x=\""sv << pos_.x << '"' << " y=\""sv << pos_.y << '"';
    out << " dx=\""sv << offset_.x << '"' << " dy=\""sv << offset_.y << '"';
    out << " font-size=\"" << font_size_ << '"';
    if (!font_family_.empty()){
        out << " font-family=\"" << font_family_ << '"';
    }
    if (!font_weight_.empty()){
        out << " font-weight=\"" << font_weight_ << '"';
    }
    RenderAttrs(context.out);
    out << '>' << data_;
    out << "</text>"sv;
}

// ---------- Document ------------------

void Document::AddPtr(std::unique_ptr<Object>&& obj){
    objects_.emplace_back(std::move(obj));
}

void Document::Render(std::ostream& out) const {
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"sv;
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n"sv;
    RenderContext ctx(out, 2, 2);
    for (auto& obj : objects_){
        obj->Render(ctx);
    }
    out << "</svg>"sv;
}


}  // namespace svg

//-----------------SHAPES----------------------

namespace shapes {

Triangle::Triangle(svg::Point p1, svg::Point p2, svg::Point p3)
: p1_(p1)
, p2_(p2)
, p3_(p3)
{}

void Triangle::Draw(svg::ObjectContainer& container) const {
    container.Add(svg::Polyline().AddPoint(p1_).AddPoint(p2_).AddPoint(p3_).AddPoint(p1_));
}

Star::Star(svg::Point center, double outer_rad, double inner_rad, int num_rays)
: star_(CreateStar(center, outer_rad, inner_rad, num_rays))
{}

void Star::Draw(svg::ObjectContainer& container) const {
    container.Add(star_);
}

Snowman::Snowman(svg::Point head_center, double radius)
: head_center_(head_center)
, radius_(radius)
{}

void Snowman::Draw(svg::ObjectContainer& container) const {
    container.Add(svg::Circle().SetCenter({head_center_.x, head_center_.y + 5 * radius_}).SetRadius(2 * radius_).SetFillColor("rgb(240,240,240)").SetStrokeColor("black"));
    container.Add(svg::Circle().SetCenter({head_center_.x, head_center_.y + 2 * radius_}).SetRadius(1.5 * radius_).SetFillColor("rgb(240,240,240)").SetStrokeColor("black"));
    container.Add(svg::Circle().SetCenter(head_center_).SetRadius(radius_).SetFillColor("rgb(240,240,240)").SetStrokeColor("black"));
}


}// namespace shapes