#if defined(__APPLE__)
#include <Security/Security.h>
#endif

#include "CredentialStore.h"

namespace Cavey {
namespace {

#if defined(__APPLE__)
struct CFReleaseDeleter {
    template <typename RefType>
    void operator()(RefType ref) const {
        if (ref != nullptr) {
            CFRelease(ref);
        }
    }
};

template <typename RefType>
using ScopedCFRef = std::unique_ptr<std::remove_pointer_t<RefType>,
                                    CFReleaseDeleter>;

CFDataRef MakeDataRef(const juce::String& value) {
    const auto utf8 = value.toRawUTF8();
    return CFDataCreate(kCFAllocatorDefault,
                        reinterpret_cast<const UInt8*>(utf8),
                        static_cast<CFIndex>(std::strlen(utf8)));
}

CFStringRef MakeStringRef(const juce::String& value) {
    return CFStringCreateWithCString(kCFAllocatorDefault,
                                     value.toRawUTF8(),
                                     kCFStringEncodingUTF8);
}

juce::String StatusToMessage(OSStatus status) {
    if (status == errSecSuccess) {
        return {};
    }

    if (auto message = SecCopyErrorMessageString(status, nullptr)) {
        juce::String result = juce::String::fromCFString(message);
        CFRelease(message);
        return result;
    }

    return "Unable to access secure credential storage.";
}
#endif

}  // namespace

bool SystemCredentialStore::saveSecret(const juce::String& account,
                                       const juce::String& secret,
                                       juce::String* error_message) {
    if (secret.isEmpty()) {
        if (error_message != nullptr) {
            *error_message = "API key cannot be empty.";
        }
        return false;
    }

#if defined(__APPLE__)
    auto service_ref = ScopedCFRef<CFStringRef>(MakeStringRef(kServiceName));
    auto account_ref = ScopedCFRef<CFStringRef>(MakeStringRef(account));
    auto secret_ref = ScopedCFRef<CFDataRef>(MakeDataRef(secret));

    const void* query_keys[] = {
        kSecClass,
        kSecAttrService,
        kSecAttrAccount
    };
    const void* query_values[] = {
        kSecClassGenericPassword,
        service_ref.get(),
        account_ref.get()
    };
    auto query = ScopedCFRef<CFDictionaryRef>(
            CFDictionaryCreate(kCFAllocatorDefault,
                               query_keys,
                               query_values,
                               3,
                               &kCFTypeDictionaryKeyCallBacks,
                               &kCFTypeDictionaryValueCallBacks));

    const void* update_keys[] = { kSecValueData };
    const void* update_values[] = { secret_ref.get() };
    auto update = ScopedCFRef<CFDictionaryRef>(
            CFDictionaryCreate(kCFAllocatorDefault,
                               update_keys,
                               update_values,
                               1,
                               &kCFTypeDictionaryKeyCallBacks,
                               &kCFTypeDictionaryValueCallBacks));

    OSStatus status = SecItemUpdate(query.get(), update.get());
    if (status == errSecItemNotFound) {
        const void* add_keys[] = {
            kSecClass,
            kSecAttrService,
            kSecAttrAccount,
            kSecValueData
        };
        const void* add_values[] = {
            kSecClassGenericPassword,
            service_ref.get(),
            account_ref.get(),
            secret_ref.get()
        };
        auto add_query = ScopedCFRef<CFDictionaryRef>(
                CFDictionaryCreate(kCFAllocatorDefault,
                                   add_keys,
                                   add_values,
                                   4,
                                   &kCFTypeDictionaryKeyCallBacks,
                                   &kCFTypeDictionaryValueCallBacks));
        status = SecItemAdd(add_query.get(), nullptr);
    }

    if (status != errSecSuccess && error_message != nullptr) {
        *error_message = StatusToMessage(status);
    }

    return status == errSecSuccess;
#else
    juce::ignoreUnused(account, secret);
    if (error_message != nullptr) {
        *error_message = "Secure credential storage is not implemented on this platform.";
    }
    return false;
#endif
}

std::optional<juce::String> SystemCredentialStore::loadSecret(
        const juce::String& account) const {
#if defined(__APPLE__)
    auto service_ref = ScopedCFRef<CFStringRef>(MakeStringRef(kServiceName));
    auto account_ref = ScopedCFRef<CFStringRef>(MakeStringRef(account));

    const void* keys[] = {
        kSecClass,
        kSecAttrService,
        kSecAttrAccount,
        kSecReturnData,
        kSecMatchLimit
    };
    const void* values[] = {
        kSecClassGenericPassword,
        service_ref.get(),
        account_ref.get(),
        kCFBooleanTrue,
        kSecMatchLimitOne
    };
    auto query = ScopedCFRef<CFDictionaryRef>(
            CFDictionaryCreate(kCFAllocatorDefault,
                               keys,
                               values,
                               5,
                               &kCFTypeDictionaryKeyCallBacks,
                               &kCFTypeDictionaryValueCallBacks));

    CFTypeRef result = nullptr;
    const OSStatus status = SecItemCopyMatching(query.get(), &result);
    if (status != errSecSuccess || result == nullptr) {
        return std::nullopt;
    }

    auto data_ref = ScopedCFRef<CFDataRef>(
            static_cast<CFDataRef>(result));
    const auto length = CFDataGetLength(data_ref.get());
    const auto bytes = CFDataGetBytePtr(data_ref.get());
    return juce::String::fromUTF8(reinterpret_cast<const char*>(bytes),
                                  static_cast<int>(length));
#else
    juce::ignoreUnused(account);
    return std::nullopt;
#endif
}

bool SystemCredentialStore::hasSecret(const juce::String& account) const {
    return loadSecret(account).has_value();
}

}  // namespace Cavey
