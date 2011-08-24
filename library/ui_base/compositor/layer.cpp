
#include "layer.h"

#include <algorithm>

#include "base/logging.h"

#include "compositor.h"

namespace ui
{

    Layer::Layer(Compositor* compositor)
        : compositor_(compositor),
        texture_(compositor->CreateTexture()),
        parent_(NULL),
        fills_bounds_opaquely_(false) {}

    Layer::~Layer()
    {
        if(parent_)
        {
            parent_->Remove(this);
        }
        for(size_t i=0; i<children_.size(); ++i)
        {
            children_[i]->parent_ = NULL;
        }
    }

    void Layer::Add(Layer* child)
    {
        if(child->parent_)
        {
            child->parent_->Remove(child);
        }
        child->parent_ = this;
        children_.push_back(child);

        if(child->fills_bounds_opaquely())
        {
            RecomputeHole();
        }
    }

    void Layer::Remove(Layer* child)
    {
        std::vector<Layer*>::iterator i = std::find(children_.begin(),
            children_.end(), child);
        DCHECK(i != children_.end());
        children_.erase(i);
        child->parent_ = NULL;

        if(child->fills_bounds_opaquely())
        {
            RecomputeHole();
        }
    }

    void Layer::SetTransform(const gfx::Transform& transform)
    {
        transform_ = transform;

        if(parent() && fills_bounds_opaquely_)
        {
            parent()->RecomputeHole();
        }
    }

    void Layer::SetBounds(const gfx::Rect& bounds)
    {
        bounds_ = bounds;

        if(parent() && fills_bounds_opaquely_)
        {
            parent()->RecomputeHole();
        }
    }

    void Layer::SetFillsBoundsOpaquely(bool fills_bounds_opaquely)
    {
        if(fills_bounds_opaquely_ == fills_bounds_opaquely)
        {
            return;
        }

        fills_bounds_opaquely_ = fills_bounds_opaquely;

        if(parent())
        {
            parent()->RecomputeHole();
        }
    }

    void Layer::SetTexture(Texture* texture)
    {
        if(texture == NULL)
        {
            texture_ = compositor_->CreateTexture();
        }
        else
        {
            texture_ = texture;
        }
    }

    void Layer::SetCanvas(const SkCanvas& canvas, const gfx::Point& origin)
    {
        texture_->SetCanvas(canvas, origin, bounds_.size());
    }

    void Layer::Draw()
    {
        ui::TextureDrawParams texture_draw_params;
        for(Layer* layer=this; layer; layer=layer->parent_)
        {
            texture_draw_params.transform.ConcatTransform(layer->transform_);
            texture_draw_params.transform.ConcatTranslate(
                static_cast<float>(layer->bounds_.x()),
                static_cast<float>(layer->bounds_.y()));
        }

        // Only blend for transparent child layers.
        // The root layer will clobber the cleared bg.
        texture_draw_params.blend = parent_ != NULL && !fills_bounds_opaquely_;

        texture_->Draw(texture_draw_params);
    }

    void Layer::DrawRegion(const ui::TextureDrawParams& params,
        const gfx::Rect& region_to_draw)
    {
        if(!region_to_draw.IsEmpty())
        {
            texture_->Draw(params, region_to_draw);
        }
    }

    void Layer::RecomputeHole()
    {
        for(size_t i=0; i<children_.size(); ++i)
        {
            if(children_[i]->fills_bounds_opaquely() &&
                !children_[i]->transform().HasChange())
            {
                hole_rect_ = children_[i]->bounds();
                return;
            }
        }
        // no opaque child layers, set hole_rect_ to empty
        hole_rect_ = gfx::Rect();
    }

} //namespace ui