use matrix_sdk_common::ruma::events::room::JsonWebKey;
use matrix_sdk_common::ruma::events::room::JsonWebKeyInit;
use matrix_sdk_common::ruma::serde::Base64;
use matrix_sdk_crypto::AttachmentDecryptor;
use std::collections::BTreeMap;
use std::error::Error;
use std::io::Read;
use log::error;

pub(crate) struct MediaEncryptionInfo {
    pub(crate) info: matrix_sdk_crypto::MediaEncryptionInfo,
    pub(crate) ciphertext: Vec<u8>,
}

impl MediaEncryptionInfo {
    pub(crate) fn media_encryption_info_bytes(&self) -> Vec<u8> {
        self.ciphertext.clone()
    }

    pub(crate) fn media_encryption_info_iv(&self) -> String {
        self.info.iv.to_string()
    }

    pub(crate) fn media_encryption_info_hash(&self) -> String {
        self.info.hashes["sha256"].to_string()
    }

    pub(crate) fn media_encryption_info_key(&self) -> String {
        self.info.key.k.to_string()
    }
}

pub(crate) fn encrypt_file(bytes: &mut [u8]) -> Box<MediaEncryptionInfo> {
    use matrix_sdk_crypto::AttachmentEncryptor;
    use std::io::Read;
    let mut binding = bytes.as_ref();
    let mut encryptor = AttachmentEncryptor::new(&mut binding);
    let mut ciphertext = vec![];
    let _ = encryptor.read_to_end(&mut ciphertext).unwrap();
    Box::new(MediaEncryptionInfo {
        info: encryptor.finish(),
        ciphertext,
    })
}

pub(crate) enum DecryptResult {
    Ok(Vec<u8>),
    Err(Box<dyn Error>),
}

impl DecryptResult {
    pub(crate) fn has_error(&self) -> bool {
        matches!(self, Self::Err(_))
    }

    pub(crate) fn error_string(&self) -> String {
        if let Self::Err(error) = self {
            error.to_string()
        } else {
            Default::default()
        }
    }

    pub(crate) fn value(&self) -> Vec<u8> {
        if let Self::Ok(value) = self {
            value.to_vec()
        } else {
            panic!()
        }
    }
}

pub(crate) fn decrypt_file(
    bytes: &mut [u8],
    iv: String,
    key: String,
    hash: String,
) -> Box<DecryptResult> {
    let fun = || {
        let mut binding = bytes.as_ref();
        let mut decryptor = AttachmentDecryptor::new(
            &mut binding,
            matrix_sdk_crypto::MediaEncryptionInfo {
                version: "v2".to_string(),
                key: JsonWebKey::from(JsonWebKeyInit {
                    alg: "A256CTR".to_string(),
                    ext: true,
                    k: Base64::parse(key)?,
                    key_ops: vec!["encrypt".to_string(), "decrypt".to_string()],
                    kty: "oct".to_string(),
                }),
                iv: Base64::parse(iv)?,
                hashes: {
                    let mut hashes: BTreeMap<String, Base64> = BTreeMap::new();
                    hashes.insert("sha256".to_string(), Base64::parse(hash)?);
                    hashes
                },
            },
        )?;
        let mut data = vec![];
        decryptor.read_to_end(&mut data)?;
        Ok(data)
    };
    let result: Result<Vec<u8>, Box<dyn Error>> = fun();

    Box::new(match result {
        Ok(value) => DecryptResult::Ok(value),
        Err(error) => {
            error!("Failed to decrypt file: {:?}", error);
            DecryptResult::Err(error)
        }
    })
}
