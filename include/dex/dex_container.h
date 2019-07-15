//
// Created by xiaobaiyey on 2019-07-14.
//

#ifndef WAAPOLICY_DEX_CONTAINER_H
#define WAAPOLICY_DEX_CONTAINER_H

#include <vector>

namespace art {

    class DexContainer {
    public:
        virtual ~DexContainer() {}

        class Section {
        public:
            virtual ~Section() {}

            // Returns the start of the memory region.
            virtual uint8_t *Begin() = 0;

            // Size in bytes.
            virtual size_t Size() const = 0;

            // Resize the backing storage.
            virtual void Resize(size_t size) = 0;

            // Clear the container.
            virtual void Clear() = 0;

            // Returns the end of the memory region.
            uint8_t *End() {
                return Begin() + Size();
            }
        };

        // Vector backed section.
        class VectorSection : public Section {
        public:
            virtual ~VectorSection() {}

            uint8_t *Begin() override {
                return &data_[0];
            }

            size_t Size() const override {
                return data_.size();
            }

            void Resize(size_t size) override {
                data_.resize(size, 0u);
            }

            void Clear() override {
                data_.clear();
            }

        private:
            std::vector<uint8_t> data_;
        };

        virtual Section *GetMainSection() = 0;

        virtual Section *GetDataSection() = 0;

        virtual bool IsCompactDexContainer() const = 0;
    };
}

#endif //WAAPOLICY_DEX_CONTAINER_H
