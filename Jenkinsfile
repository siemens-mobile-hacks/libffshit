pipeline {
    agent none
    stages {
        stage('Ubuntu build') {
            agent {
                node {
                    label 'ubuntu'
                }
            }
            environment {
                GIT_COMMIT_SHORT = sh(
                        script: "printf \$(git rev-parse --short ${GIT_COMMIT})",
                        returnStdout: true
                )
                BUILD_TIMESTAMP = sh(script: "echo `date +%Y-%m-%d_%H-%M-%S`", returnStdout: true).trim()
                TAR_DEBUG_NAME = "libffshit-${BUILD_TIMESTAMP}-linux-amd64-debug-${GIT_COMMIT_SHORT}"
                TAR_RELEASE_NAME = "libffshit-${BUILD_TIMESTAMP}-linux-amd64-release-${GIT_COMMIT_SHORT}"
            }
            stages {
                stage('Build debug') {
                    stages {
                        stage('Build') {
                            steps {
                                sh 'rm -rf build'
                                sh 'cmake -DCMAKE_BUILD_TYPE=Debug -B build -S .' 
                                sh 'cmake --build build --config Debug'
                            }
                        }
                        stage('Install') {
                            steps {
                                sh 'rm -rf target'
                                sh 'rm -f ${TAR_DEBUG_NAME}.tar.gz'
                                sh 'cmake --install build --prefix target'
                                sh 'tar cf ${TAR_DEBUG_NAME}.tar target/'
                                sh 'gzip ${TAR_DEBUG_NAME}.tar'
                                archiveArtifacts artifacts: "${env.TAR_DEBUG_NAME}.tar.gz", fingerprint: true
                            }
                        }
                    }
                }
                stage('Build release') {
                    stages {
                        stage('Build') {
                            steps {
                                sh 'rm -rf build'
                                sh 'cmake -DCMAKE_BUILD_TYPE=Release -B build -S .' 
                                sh 'cmake --build build --config Release'
                            }
                        }
                        stage('Install') {
                            steps {
                                sh 'rm -rf target'
                                sh 'rm -f ${TAR_RELEASE_NAME}.tar.gz'
                                sh 'cmake --install build --prefix target'
                                sh 'tar cf ${TAR_RELEASE_NAME}.tar target/'
                                sh 'gzip ${TAR_RELEASE_NAME}.tar'
                                archiveArtifacts artifacts: "${env.TAR_RELEASE_NAME}.tar.gz", fingerprint: true
                            }
                        }
                    }
                }

            }
        }
        stage('macOS build arm64') {
            agent {
                node {
                    label 'macos-arm64'
                }
            }
            environment {
                GIT_COMMIT_SHORT = sh(
                        script: "printf \$(git rev-parse --short ${GIT_COMMIT})",
                        returnStdout: true
                )
                BUILD_TIMESTAMP = sh(script: "echo `date +%Y-%m-%d_%H-%M-%S`", returnStdout: true).trim()
                TAR_DEBUG_NAME = "libffshit-${BUILD_TIMESTAMP}-macos-arm64-debug-${GIT_COMMIT_SHORT}"
                TAR_RELEASE_NAME = "libffshit-${BUILD_TIMESTAMP}-macos-arm64-release-${GIT_COMMIT_SHORT}"
            }
            stages {
                stage('Build debug') {
                    stages {
                        stage('Build') {
                            steps {
                                sh 'rm -rf build'
                                sh 'cmake -DCMAKE_BUILD_TYPE=Debug -B build -S .' 
                                sh 'cmake --build build --config Debug'
                            }
                        }
                        stage('Install') {
                            steps {
                                sh 'rm -rf target'
                                sh 'rm -f ${TAR_DEBUG_NAME}.tar.gz'
                                sh 'cmake --install build --prefix target'
                                sh 'tar cf ${TAR_DEBUG_NAME}.tar target/'
                                sh 'gzip ${TAR_DEBUG_NAME}.tar'
                                archiveArtifacts artifacts: "${env.TAR_DEBUG_NAME}.tar.gz", fingerprint: true
                            }
                        }
                    }
                }
                stage('Build release') {
                    stages {
                        stage('Build') {
                            steps {
                                sh 'rm -rf build'
                                sh 'cmake -DCMAKE_BUILD_TYPE=Release -B build -S .' 
                                sh 'cmake --build build --config Release'
                            }
                        }
                        stage('Install') {
                            steps {
                                sh 'rm -rf target'
                                sh 'rm -f ${TAR_RELEASE_NAME}.tar.gz'
                                sh 'cmake --install build --prefix target'
                                sh 'tar cf ${TAR_RELEASE_NAME}.tar target/'
                                sh 'gzip ${TAR_RELEASE_NAME}.tar'
                                archiveArtifacts artifacts: "${env.TAR_RELEASE_NAME}.tar.gz", fingerprint: true
                            }
                        }
                    }
                }       
            }     
        }
    }
}
