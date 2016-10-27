/** Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include "oio/rawx/blob.h"

#include <cassert>
#include <string>
#include <functional>
#include <sstream>
#include <memory>
#include <vector>
#include <iomanip>
#include <cstring>

#include "utils/macros.h"
#include "utils/net.h"
#include "utils/Http.h"
#include "oio/api/blob.h"

using oio::rawx::blob::RemovalBuilder;
using oio::rawx::blob::UploadBuilder;
using oio::rawx::blob::DownloadBuilder;
using oio::api::blob::Removal;
using oio::api::blob::Upload;
using oio::api::blob::Download;
using oio::api::blob::Status;
using oio::api::blob::Cause;
using Step = oio::api::blob::TransactionStep;

/**
 *
 */
class RawxRemoval : public Removal {
    friend class RemovalBuilder;

 private:
    std::unique_ptr<Removal> inner;
    Step step_;

 public:
    explicit RawxRemoval(Removal *sub) : inner(sub), step_{Step::Init} {}

    virtual ~RawxRemoval() {}

    Status Prepare() override {
        if (step_ != Step::Init)
            return Status(Cause::InternalError);
        auto rc = inner->Prepare();
        if (rc.Ok())
            step_ = Step::Prepared;
        return rc;
    }

    Status Commit() override {
        if (step_ != Step::Prepared)
            return Status(Cause::InternalError);
        step_ = Step::Done;
        return inner->Commit();
    }

    Status Abort() override {
        if (step_ != Step::Prepared)
            return Status(Cause::InternalError);
        step_ = Step::Done;
        return inner->Abort();
    }
};

RemovalBuilder::RemovalBuilder() {}

RemovalBuilder::~RemovalBuilder() {}

void RemovalBuilder::ChunkId(const std::string &s) {
    return inner.Name("/rawx/" + s);
}

void RemovalBuilder::RawxId(const std::string &s) { return inner.Host(s); }

std::unique_ptr<Removal> RemovalBuilder::Build(
        std::shared_ptr<net::Socket> socket) {
    auto sub = inner.Build(socket);
    auto rem = new RawxRemoval(sub.release());
    return std::unique_ptr<Removal>(rem);
}


/**
 *
 */
class RawxUpload : public Upload {
    friend class UploadBuilder;
 private:
    std::unique_ptr<Upload> inner;
    Step step_;

 public:
    explicit RawxUpload(Upload *sub) : inner(sub), step_{Step::Init} {
        assert(sub != nullptr);
    }

    ~RawxUpload() { }

    void SetXattr(const std::string &k, const std::string &v) override {
        return inner->SetXattr(k, v);
    }

    Status Prepare() override {
        if (step_ != Step::Init)
            return Status(Cause::InternalError);
        auto rc = inner->Prepare();
        if (rc.Ok())
            step_ = Step::Prepared;
        return rc;
    }

    Status Commit() override {
        if (step_ != Step::Prepared)
            return Status(Cause::InternalError);
        step_ = Step::Done;
        return inner->Commit();
    }

    Status Abort() override {
        if (step_ != Step::Prepared)
            return Status(Cause::InternalError);
        step_ = Step::Done;
        return inner->Abort();
    }

    void Write(const uint8_t *buf, uint32_t len) override {
        return inner->Write(buf, len);
    }
};

UploadBuilder::UploadBuilder() {}

UploadBuilder::~UploadBuilder() {}

void UploadBuilder::RawxId(const std::string &s) { inner.Host(s); }

void UploadBuilder::ChunkId(const std::string &s) {
    inner.Field("X-oio-chunk-meta-chunk-id", s);
    inner.Name("/rawx/" + s);
}

void UploadBuilder::ChunkPosition(int64_t meta, int64_t sub) {
    std::stringstream ss;
    ss << meta << '.' << sub;
    inner.Field("X-oio-chunk-meta-chunk-pos", ss.str());
}

void UploadBuilder::ContainerId(const std::string &s) {
    inner.Field("X-oio-chunk-meta-container-id", s);
}

void UploadBuilder::ContentPath(const std::string &s) {
    inner.Field("X-oio-chunk-meta-content-path", s);
}

void UploadBuilder::ContentId(const std::string &s) {
    inner.Field("X-oio-chunk-meta-content-id", s);
}

void UploadBuilder::MimeType(const std::string &s) {
    inner.Field("X-oio-chunk-meta-content-mime-type", s);
}

void UploadBuilder::StoragePolicy(const std::string &s) {
    inner.Field("X-oio-chunk-meta-content-storage-policy", s);
}

void UploadBuilder::ChunkMethod(const std::string &s) {
    inner.Field("X-oio-chunk-meta-content-chunk-method", s);
}

void UploadBuilder::ContentVersion(int64_t v) {
    std::stringstream ss;
    ss << v;
    inner.Field("X-oio-chunk-meta-content-version", ss.str());
}

void UploadBuilder::Property(const std::string &k, const std::string &v) {
    inner.Field("X-oio-chunk-meta-x-" + k, v);
}

std::unique_ptr<Upload> UploadBuilder::Build(
        std::shared_ptr<net::Socket> socket) {
    auto sub = inner.Build(socket);
    assert(sub.get() != nullptr);
    auto ul = new RawxUpload(sub.release());
    return std::unique_ptr<Upload>(ul);
}


/**
 *
 */
class RawxDownload : public Download {
    friend class DownloadBuilder;

 private:
    std::unique_ptr<Download> inner;
    Step step_;

 public:
    explicit RawxDownload(Download *sub) : inner(sub), step_{Step::Init} {}

    ~RawxDownload() {}

    bool IsEof() override { return inner->IsEof(); }

    Status Prepare() override {
        if (step_ != Step::Init)
            return Status(Cause::InternalError);
        auto rc = inner->Prepare();
        if (rc.Ok())
            step_ = Step::Prepared;
        return rc;
    }

    int32_t Read(std::vector<uint8_t> *buf) override {
        if (step_ != Step::Prepared)
            return -1;
        return inner->Read(buf);
    }
};

DownloadBuilder::DownloadBuilder() {}

DownloadBuilder::~DownloadBuilder() {}

void DownloadBuilder::RawxId(const std::string &s) { return inner.Host(s); }

void DownloadBuilder::ChunkId(const std::string &s) {
    return inner.Name("/rawx/" + s);
}

std::unique_ptr<Download> DownloadBuilder::Build(
        std::shared_ptr<net::Socket> socket) {
    auto sub = inner.Build(socket);
    auto dl = new RawxDownload(sub.release());
    return std::unique_ptr<Download>(dl);
}
